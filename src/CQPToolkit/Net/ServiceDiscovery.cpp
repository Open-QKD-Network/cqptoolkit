/*!
* @file
* @brief ServiceDiscovery
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/9/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ServiceDiscovery.h"
#include "CQPToolkit/Util/Logger.h"
#include "Net/Interface.h"
#include "CQPToolkit/Interfaces/IService.h"
#include "CQPToolkit/Util/UUID.h"
#include "CQPToolkit/Util/WorkerThread.h"
#include "CQPToolkit/Util/Util.h"

#if defined(AVAHI_FOUND)
    #include <avahi-common/alternative.h>
    #include <avahi-common/malloc.h>
    #include <avahi-common/error.h>
    #include <avahi-common/timeval.h>

    #include <avahi-client/client.h>
    #include <avahi-client/publish.h>
    #include <avahi-common/simple-watch.h>
    #include <avahi-client/lookup.h>
#endif

namespace cqp
{
    namespace net
    {
        const std::string interfacesString = "Interfaces=";
        const std::string idString = "ID=";

#if defined(AVAHI_FOUND)
        class ServiceDiscoveryImpl : public WorkerThread
        {
        public:
            explicit ServiceDiscoveryImpl(ServiceDiscovery* p);

            ~ServiceDiscoveryImpl() override;

            void ResetGroup();

            void DoWork() override;
            void HandleCollision(RemoteHost& service);
            void CreateServices();

        protected:
            static void AvahiClientCallback(AvahiClient* c, AvahiClientState state, void * userdata);
            static void AvahiEntryGroupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata);
            static void AvahiModifyCallback(AVAHI_GCC_UNUSED AvahiTimeout *e, void *userdata);
            static void AvahiResolveCallback(
                AvahiServiceResolver *r,
                AVAHI_GCC_UNUSED AvahiIfIndex interface,
                AVAHI_GCC_UNUSED AvahiProtocol protocol,
                AvahiResolverEvent event,
                const char *name,
                const char *type,
                const char *domain,
                const char *host_name,
                const AvahiAddress *address,
                uint16_t port,
                AvahiStringList *txt,
                AvahiLookupResultFlags flags,
                AVAHI_GCC_UNUSED void* userdata);
            static void AvahiBrowseCallback(AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event,
                                            const char *name, const char *type, const char *domain, AvahiLookupResultFlags flags, void *userdata);

            const std::string serviceType = "_grpc._tcp";

            ServiceDiscovery* parent = nullptr;
            AvahiEntryGroup *group = nullptr;
            AvahiSimplePoll *simple_poll = nullptr;
            AvahiClient *client = nullptr;
            AvahiServiceBrowser *sb = nullptr;
            ////// C state variables

            std::string serviceName;
        };

        void ServiceDiscoveryImpl::AvahiEntryGroupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata)
        {
            ServiceDiscoveryImpl* self = static_cast<ServiceDiscoveryImpl*>(userdata);

            if(self)
            {
                /* Called whenever the entry group state changes */
                switch (state)
                {
                case AVAHI_ENTRY_GROUP_ESTABLISHED :
                    /* The entry group has been established successfully */
                    LOGINFO("Service successfully established.");
                    break;
                case AVAHI_ENTRY_GROUP_COLLISION :
                {
                    char *newName;
                    /* A service name collision with a remote service
                     * happened. Let's pick a new name */
                    newName = avahi_alternative_service_name(self->serviceName.c_str());
                    self->serviceName = newName;
                    avahi_free(newName);
                    LOGINFO("Service name collision, renaming service to " + self->serviceName);
                    /* And recreate the services */
                    self->CreateServices();
                    break;
                }
                case AVAHI_ENTRY_GROUP_FAILURE :
                    LOGINFO("Entry group failure: " + avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
                    /* Some kind of failure happened while we were registering our services */
                    avahi_simple_poll_quit(self->simple_poll);
                    break;
                case AVAHI_ENTRY_GROUP_UNCOMMITED:
                case AVAHI_ENTRY_GROUP_REGISTERING:
                    ;
                }
            } // if(self)
            else
            {
                LOGERROR("Invalid userdata, no instance provided");
            } // if(self)
        } // AvahiEntryGroupCallback

        void ServiceDiscoveryImpl::CreateServices()
        {
            try
            {

                /* If this is the first time we're called, let's create a new
                 * entry group if necessary */
                if (client && !group)
                {
                    if (!(group = avahi_entry_group_new(client, ServiceDiscoveryImpl::AvahiEntryGroupCallback, this)))
                    {
                        LOGERROR("avahi_entry_group_new() failed: " + avahi_strerror(avahi_client_errno(client)));
                    }
                }
                /* If the group is empty (either because it was just created, or
                 * because it was reset previously, add our entries.  */
                if (group && avahi_entry_group_is_empty(group))
                {
                    int ret = 0;

                    /* We will now add two services and one subtype to the entry
                     * group. The two services have the same name, but differ in
                     * the service type (IPP vs. BSD LPR). Only services with the
                     * same name should be put in the same entry group. */
                    /* Add the service for IPP */
                    using namespace std;
                    lock_guard<mutex> lock(parent->changeMutex);

                    for(auto& service : parent->myServices)
                    {
                        LOGINFO("Adding service " + service.name);
                        serviceName = service.name;
                        std::string idRecord = idString + service.id;
                        std::string interfacesRecord = interfacesString;

                        for(auto iface : service.interfaces)
                        {
                            interfacesRecord += iface + ";";
                        }

                        if ((ret = avahi_entry_group_add_service(group,
                                   AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0),
                                   /*name*/ service.name.c_str(), /*type*/ serviceType.c_str(),
                                   /*domain*/ nullptr, /*host*/ service.host.c_str(), /*port*/ service.port,
                                   idRecord.c_str(),
                                   interfacesRecord.c_str(),
                                   /*parameters end*/ NULL)) < 0)
                        {
                            if (ret == AVAHI_ERR_COLLISION)
                            {
                                HandleCollision(service);
                            }
                            else
                            {
                                LOGERROR("Failed to add service: " + avahi_strerror(ret));
                                avahi_simple_poll_quit(simple_poll);
                            }
                        } // if
                    } // for

                    /* Tell the server to register the service */
                    if ((ret = avahi_entry_group_commit(group)) < 0)
                    {
                        LOGERROR("Failed to commit entry group: " + avahi_strerror(ret));
                        avahi_simple_poll_quit(simple_poll);
                    } // if
                } // if

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        } // CreateServices

        ServiceDiscoveryImpl::ServiceDiscoveryImpl(ServiceDiscovery* p) : parent(p)
        {
            int error;
            try
            {
                /* Allocate main loop object */
                if (!(simple_poll = avahi_simple_poll_new()))
                {
                    LOGERROR("Failed to create simple poll object.\n");
                }
                else
                {
                    /* Allocate a new client */
                    client = avahi_client_new(avahi_simple_poll_get(simple_poll), AvahiClientFlags(0), &ServiceDiscoveryImpl::AvahiClientCallback, this, &error);
                    /* Check wether creating the client object succeeded */
                    if (!client)
                    {
                        LOGERROR("Failed to create client: " + avahi_strerror(error));
                    } // if (!client)
                    else
                    {
                        Start();
                        sb = avahi_service_browser_new(client,
                                                       AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
                                                       serviceType.c_str(), nullptr, AvahiLookupFlags(0), ServiceDiscoveryImpl::AvahiBrowseCallback, this);
                        if(!sb)
                        {
                            LOGERROR("Failed to create service browser: " + avahi_strerror(avahi_client_errno(client)));
                        }
                    } // if (!client)
                } // if
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }

        ServiceDiscoveryImpl::~ServiceDiscoveryImpl()
        {
            Stop(true);

            if (sb)
            {
                avahi_service_browser_free(sb);
            } // if (sb)

            /* Cleanup things */
            if (client)
            {
                avahi_client_free(client);
            } // if (client)

            if (simple_poll)
            {
                avahi_simple_poll_free(simple_poll);
            } // if (simple_poll)
        }

        void ServiceDiscoveryImpl::ResetGroup()
        {
            if(group)
            {
                avahi_entry_group_reset(group);
            }
        }

        void ServiceDiscoveryImpl::HandleCollision(RemoteHost& service)
        {
            try
            {
                char* newName = avahi_alternative_service_name(service.id.c_str());
                LOGWARN("Service name collision, renaming service to " + newName);
                service.id = newName;
                avahi_free(newName);
                avahi_entry_group_reset(group);
                CreateServices();
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        } // HandleCollision

        void ServiceDiscoveryImpl::AvahiClientCallback(AvahiClient *c, AvahiClientState state, void * userdata)
        {
            ServiceDiscoveryImpl* self = static_cast<ServiceDiscoveryImpl*>(userdata);
            if(self && c)
            {
                /* Called whenever the client or server state changes */
                switch (state)
                {
                case AVAHI_CLIENT_S_RUNNING:
                    /* The server has startup successfully and registered its host
                    * name on the network, so it's time to create our services */
                    self->CreateServices();
                    break;
                case AVAHI_CLIENT_FAILURE:
                    LOGERROR("Client failure: " + avahi_strerror(avahi_client_errno(c)));
                    avahi_simple_poll_quit(self->simple_poll);
                    break;
                case AVAHI_CLIENT_S_COLLISION:
                /* Let's drop our registered services. When the server is back
                * in AVAHI_SERVER_RUNNING state we will register them
                * again with the new host name. */
                case AVAHI_CLIENT_S_REGISTERING:
                    /* The server records are now being established. This
                    * might be caused by a host name change. We need to wait
                    * for our own records to register until the host name is
                    * properly esatblished. */
                    if (self->group)
                        avahi_entry_group_reset(self->group);
                    break;
                case AVAHI_CLIENT_CONNECTING:
                    ;
                } // switch (state)
            } // if(self && c)
        } // AvahiClientCallback

        void ServiceDiscoveryImpl::AvahiModifyCallback(AVAHI_GCC_UNUSED AvahiTimeout *e, void *userdata)
        {
            try
            {
                ServiceDiscoveryImpl *self = static_cast<ServiceDiscoveryImpl *>(userdata);

                // do modification

                /* If the server is currently running, we need to remove our
                 * service and create it anew */
                if (avahi_client_get_state(self->client) == AVAHI_CLIENT_S_RUNNING)
                {
                    /* Remove the old services */
                    if (self->group)
                        avahi_entry_group_reset(self->group);
                    /* And create them again with the new name */
                    self->CreateServices();
                } // if

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        } // AvahiModifyCallback

        void ServiceDiscoveryImpl::DoWork()
        {
            try
            {
                int result = avahi_simple_poll_iterate(simple_poll, 1000);
                if(result < 0)
                {
                    LOGERROR("Avahi polling failed.");
                } // if(result < 0)
                else if(result > 0)
                {
                    Stop(false);
                } // if(result > 0)
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        } // AvahiThread

        void ServiceDiscoveryImpl::AvahiResolveCallback(
            AvahiServiceResolver *r,
            AVAHI_GCC_UNUSED AvahiIfIndex interface,
            AVAHI_GCC_UNUSED AvahiProtocol protocol,
            AvahiResolverEvent event,
            const char *name,
            const char *type,
            const char *domain,
            const char *host_name,
            const AvahiAddress *address,
            uint16_t port,
            AvahiStringList *txtRecords,
            AvahiLookupResultFlags flags,
            void* userdata)
        {
            ServiceDiscoveryImpl *self = static_cast<ServiceDiscoveryImpl *>(userdata);
            if(self && r)
            {
                /* Called whenever a service has been resolved successfully or timed out */
                switch (event)
                {
                case AVAHI_RESOLVER_FAILURE:
                    LOGERROR("(Resolver) Failed to resolve service '" + name + "' of type '" + type + "' in domain '" + domain + "': " + avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))) + "\n");
                    break;
                case AVAHI_RESOLVER_FOUND:
                {
                    char a[AVAHI_ADDRESS_STR_MAX] {}; /* FlawFinder: ignore */
                    char* txtString {};
                    LOGTRACE("Service '" + name + "' of type '" + type + "' in domain '" + domain + "':\n");
                    avahi_address_snprint(a, sizeof(a), address);
                    txtString = avahi_string_list_to_string(txtRecords);
                    LOGTRACE(
                        "\t" + host_name + ":" + std::to_string(port) + " (" + a + ")\n" +
                        "\tTXT=" + txtString + "\n" +
                        "\ttcookie is " + std::to_string(avahi_string_list_get_service_cookie(txtRecords)) + "\n" +
                        "\tis_local: " + std::to_string(!!(flags & AVAHI_LOOKUP_RESULT_LOCAL)) + "\n" +
                        "\tour_own: " + std::to_string(!!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN)) + "\n"
                        "\twide_area: " + std::to_string(!!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA)) + "\n"
                        "\tmulticast: " + std::to_string(!!(flags & AVAHI_LOOKUP_RESULT_MULTICAST)) + "\n"
                        "\tcached: " + std::to_string(!!(flags & AVAHI_LOOKUP_RESULT_CACHED)) + "\n");

                    RemoteHosts addedServices;

                    {
                        using namespace std;
                        lock_guard<mutex> lock(self->parent->changeMutex);

                        RemoteHost& entry = self->parent->services[name];
                        entry.host = host_name;
                        entry.name = name;
                        entry.port = port;

                        for(auto record = txtRecords; record != nullptr; record = record->next)
                        {
                            std::string recordStr(reinterpret_cast<char*>(record->text));
                            // if this is the interfaces record
                            if(recordStr.compare(0, interfacesString.length(), interfacesString) == 0)
                            {
                                SplitString(recordStr, entry.interfaces, ";", interfacesString.length());
                            }
                            else if(recordStr.compare(0, idString.length(), idString) == 0)
                            {
                                entry.id = recordStr.substr(idString.length());
                            }
                        }

                        addedServices[name] = entry;
                    }

                    avahi_free(txtString);

                    self->parent->Emit(addedServices, RemoteHosts());
                }
                } // switch (event)
            } // if(self && r)
            avahi_service_resolver_free(r);
        } // AvahiResolveCallback

        void ServiceDiscoveryImpl::AvahiBrowseCallback(AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event,
                const char *name, const char *type, const char *domain, AvahiLookupResultFlags, void *userdata)
        {
            ServiceDiscoveryImpl *self = static_cast<ServiceDiscoveryImpl *>(userdata);

            if(self && b)
            {
                /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
                switch (event)
                {
                case AVAHI_BROWSER_FAILURE:
                    LOGERROR("(Browser) " + avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
                    avahi_simple_poll_quit(self->simple_poll);
                    return;
                case AVAHI_BROWSER_NEW:
                    LOGDEBUG("(Browser) NEW: service " + name + " of type " + type + " in domain " + domain);
                    /* We ignore the returned resolver object. In the callback
                       function we free it. If the server is terminated before
                       the callback function is called the server will free
                       the resolver for us. */
                    if (!(avahi_service_resolver_new(self->client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AvahiLookupFlags(0), self->AvahiResolveCallback, self)))
                        LOGERROR("Failed to resolve service '" + name + "': " + avahi_strerror(avahi_client_errno(self->client)));
                    break;
                case AVAHI_BROWSER_REMOVE:
                {
                    LOGDEBUG("(Browser) REMOVE: service '" + name + "' of type '" + type + "' in domain '" + domain + "'");
                    using namespace std;
                    lock_guard<mutex> lock(self->parent->changeMutex);

                    RemoteHosts removedServices;
                    removedServices[name] = self->parent->services[name];
                    self->parent->services.erase(name);
                    self->parent->Emit(RemoteHosts(), removedServices);
                }
                break;
                case AVAHI_BROWSER_ALL_FOR_NOW:
                    LOGTRACE("(Browser) ALL_FOR_NOW");
                    break;
                case AVAHI_BROWSER_CACHE_EXHAUSTED:
                    LOGTRACE("(Browser) CACHE_EXHAUSTED");
                    break;
                } // switch (event)
            } // if(self && b)
            else
            {
                LOGERROR("Invalid user data provided to callback");
            } // if(self && b)
        } // AvahiBrowseCallback

#else
        class ServiceDiscoveryImpl : public WorkerThread
        {
        public:
            explicit ServiceDiscoveryImpl(ServiceDiscovery* p) {};

            ~ServiceDiscoveryImpl() override {};

            void ResetGroup() {};

            void DoWork() override {};
            void HandleCollision(RemoteHost& service) {};
            void CreateServices() {};
        };
#endif
        ServiceDiscovery::ServiceDiscovery()
        {
            impl = new ServiceDiscoveryImpl(this);

        } // ServiceDiscovery

        ServiceDiscovery::~ServiceDiscovery()
        {
            try
            {
                impl->Stop(true);
                delete(impl);
                impl = nullptr;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        } // ~ServiceDiscovery

        void ServiceDiscovery::Add(IServiceCallback *listener)
        {
            using namespace std;
            lock_guard<mutex> lock(changeMutex);

            if (services.size() > 0 && listener)
            {
                // we only send updates when the list changes
                // send the current list to the new client
                listener->OnServiceDetected(services, RemoteHosts());
            }
            // add the listener as normal
            ServiceDiscoveryEvent::Add(listener);
        } // Add

        void ServiceDiscovery::SetServices(const RemoteHost &details)
        {
            // Store the service in the map, any existing service will be overwritten
            myServices.push_back(details);
            impl->ResetGroup();
            impl->CreateServices();
        } // SetServices

    } // namespace net
} // namespace cqp
