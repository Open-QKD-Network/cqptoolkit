/*!
* @file
* @brief ISessionController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/Base.h"
#include "Algorithms/Datatypes/URI.h"
#include <grpc++/security/server_credentials.h>

namespace grpc {
    class Status;
    template<typename>
    class ServerWriter;
    class ServerContext;
}

namespace cqp
{
    namespace remote {
        class LinkStatus;
        class SessionDetails;
    }

    /**
     * @brief The ISessionController class
     * Interface for managing key generation between two points
     */
    class CQPTOOLKIT_EXPORT ISessionController
    {
    public:

        /**
         * @brief StartService
         * Start the service and listen for client connections
         * @param hostname optional address to advertise to other clients for them to connect to this server
         * @param listenPort optional port to attach to, if 0, a port will be chosen at random
         * @param creds Security credentials used for connections
         * @return status of the connection
         */
        virtual grpc::Status StartServer(const std::string& hostname = "", uint16_t listenPort = 0, std::shared_ptr<grpc::ServerCredentials> creds = grpc::InsecureServerCredentials()) = 0;

        /**
         * @brief StartServiceAndConnect
         * The same as StartServer, but also connect to a running service
         * @param otherController The other side to connect to
         * @param hostname specify hostname to override the default
         * @param listenPort specify to override the default
         * @param creds Security credentials used for connections
         * @return status of the connection
         */
        virtual grpc::Status StartServerAndConnect(URI otherController, const std::string& hostname = "", uint16_t listenPort = 0, std::shared_ptr<grpc::ServerCredentials> creds = grpc::InsecureServerCredentials()) = 0;

        /**
         * @brief Connect
         * Connect to a running service, requires that StartServer has previously been called
         * @param otherController The other side to connect to
         * @return status of the connection
         */
        virtual grpc::Status Connect(URI otherController) = 0;

        /**
         * @brief GetListenPort
         * @return The port number for connecting
         */
        virtual std::string GetConnectionAddress() const = 0;

        /// @copydoc remote::IDevice::GetLinkStatus
        virtual grpc::Status GetLinkStatus(grpc::ServerContext* context, ::grpc::ServerWriter<remote::LinkStatus>* writer) = 0;

        /**
         * @brief StartSession
         * Instruct both sides to start generating key
         * From this point on, GetKeyPublisher can be called
         * @details
         * @startuml StartSessionBehaviour
         *    control "SessionController : A" as SeshA
         *    boundary "ISessionController : B" as SeshB
         *
         *    [-> SeshA : StartSession
         *    activate SeshA
         *        note over SeshA
         *            Connect to the other session controller
         *        end note
         *        SeshA -> SeshB : SessionStarting()
         *    deactivate SeshA
         * @enduml
         * @return result of command
         */
        virtual grpc::Status StartSession(const remote::SessionDetails& sessionDetails) = 0;

        /**
         * @brief EndSession
         * Stop generating key
         */
        virtual void EndSession() = 0;

        /**
         * @brief ~ISessionController
         * Destructor
         */
        virtual ~ISessionController() = default;
    };

} // namespace cqp


