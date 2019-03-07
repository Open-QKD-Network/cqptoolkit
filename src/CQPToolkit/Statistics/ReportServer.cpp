/*!
* @file
* @brief ReportServer
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 23/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ReportServer.h"

namespace cqp
{
    namespace stats
    {
        using grpc::Status;

        template<typename T, class RPT>
        void SetReportValues(const stats::Stat<T>* stat, RPT* rpt)
        {
            rpt->set_average(stat->GetAverage());
            rpt->set_latest(stat->GetLatest());
            rpt->set_max(stat->GetMax());
            rpt->set_min(stat->GetMin());
            rpt->set_total(stat->GetTotal());
        }

        bool ReportServer::ShouldSendStat(const remote::ReportingFilter* filter, const std::chrono::system_clock::time_point& lastUpdate, const remote::SiteAgentReport& report)
        {
            using namespace std::chrono;
            bool result = false;
            if((high_resolution_clock::now() - lastUpdate) > milliseconds(filter->maxrate_ms()))
            {
                bool match = false;
                // try and find a match in each filter
                for(const auto& filter : filter->filters())
                {
                    // the name of the stat is built from a tree, eg
                    // TimeTaken -> Sifting -> QKD
                    auto currentStatname = report.path().begin();
                    auto filtername = filter.fullname().begin();
                    // drop out if there's nothing to compare
                    match = currentStatname != report.path().end() && !filter.fullname().empty();

                    // walk back through the list of names and check they match
                    // TimeTaken -> Sifting -> QKD != TimeTaken -> Alignment -> QKD
                    while(currentStatname != report.path().end() && filtername != filter.fullname().end())
                    {
                        if(*currentStatname != *filtername)
                        {
                            match = false;
                            break; // while
                        }
                        // move to the next set of names
                        currentStatname++;
                        filtername++;
                    } // while names valid

                    // a filter rule exists for this list of names
                    if(match)
                    {
                        break; // for
                    } // if match
                } // for filters

                if(filter->listisexclude())
                {
                    // item matched an exclude list
                    result = !match;
                }
                else
                {
                    // item matched an include list
                    result = match;
                }
            }

            return result;
        }

        void ReportServer::StatsReport(const remote::SiteAgentReport& report)
        {
            std::lock_guard<std::mutex> lock(listenersMutex);
            for(auto& listener : remoteListeners)
            {
                if(ShouldSendStat(&listener.second.filter, listener.second.lastUpdate, report))
                {
                    /*lock scope*/
                    {
                        std::lock_guard<std::mutex> lock(listener.second.reportMutex);
                        listener.second.reports.push(report);
                        // add our aditional properties
                        for(const auto& prop : additional)
                        {
                            (*listener.second.reports.back().mutable_parameters())[prop.first] = prop.second;
                        }
                    }/*lock scope*/

                    listener.second.reportCv.notify_one();
                } // if SendStat
            } // for listeners

            // send update to locally attached listeners
            Emit(report);
        }

        void ReportServer::AddAdditionalProperties(const std::string& key, const std::string& value)
        {
            additional[key] = value;
        }

        ReportServer::~ReportServer()
        {
            shutdown = true;
            for(auto& listener : remoteListeners)
            {
                // release the reading thread so it can check the status of shutdown
                listener.second.reportCv.notify_all();
            }

            // wait for all the listeners to quit
            std::unique_lock<std::mutex> lock(listenersMutex);
            listenerCv.wait(lock, [&](){
                return remoteListeners.empty();
            });
        }

        Status ReportServer::GetStatistics(grpc::ServerContext*,
                                           const remote::ReportingFilter* request,
                                           ::grpc::ServerWriter<remote::SiteAgentReport>* writer)
        {
            using namespace std;

            Status result;
            size_t listenerId = 0;

            /*lock scope*/
            {
                lock_guard<mutex> listenerLock(listenersMutex);
                // prep our listener settings
                listenerId = nextListenerId++;
                remoteListeners[listenerId].filter = *request;
            }/*lock scope*/

            Reportlistener& details = remoteListeners[listenerId];

            bool keepWriting = true;
            while(keepWriting && !shutdown)
            {

                unique_lock<mutex> lock(details.reportMutex);
                details.reportCv.wait(lock, [&]
                {
                    return !details.reports.empty() || shutdown;
                });

                if(!details.reports.empty())
                {
                    while(keepWriting && !details.reports.empty() && !shutdown)
                    {
                        keepWriting = writer->Write(details.reports.front());
                        details.reports.pop();
                    }
                    details.lastUpdate = std::chrono::high_resolution_clock::now();
                } // if(dataReady)
            } // while(keepWriting)

            /*lock scope*/
            {
                lock_guard<mutex> listenerLock(listenersMutex);
                // remove our listener settings
                remoteListeners.erase(listenerId);
            }/*lock scope*/
            // tell anyone waiting that we have removed a listener
            listenerCv.notify_all();

            return result;
        }

        void ReportServer::StatUpdated(const stats::Stat<double>* stat)
        {
            remote::SiteAgentReport response;
            SetReportValues<double, remote::ReportValueDouble>(stat, response.mutable_asdouble());
            CompleteReport(stat, response);
        }

        void ReportServer::StatUpdated(const stats::Stat<long>* stat)
        {
            remote::SiteAgentReport response;
            SetReportValues<long, remote::ReportValueLong>(stat, response.mutable_aslong());
            CompleteReport(stat, response);
        }

        void ReportServer::StatUpdated(const stats::Stat<size_t>* stat)
        {
            remote::SiteAgentReport response;
            SetReportValues<size_t, remote::ReportValueUnsigned>(stat, response.mutable_asunsigned());
            CompleteReport(stat, response);
        }

        void ReportServer::CompleteReport(const stats::StatBase* stat, remote::SiteAgentReport& report)
        {
            using namespace std::chrono;
            using remote::SiteAgentReport;

            report.set_rate(stat->GetRate());
            auto sinceEpoc = stat->GetUpdated().time_since_epoch();
            seconds secs = duration_cast<seconds>(sinceEpoc);
            nanoseconds remainder = duration_cast<nanoseconds>(sinceEpoc) - secs;

            report.mutable_updated()->set_seconds(
                secs.count()
            );
            report.mutable_updated()->set_seconds(
                remainder.count()
            );
            report.set_id(stat->GetId());

            for(const auto& el : stat->GetPath())
            {
                report.add_path(el);
            }

            for(const auto& param : stat->parameters)
            {
                report.mutable_parameters()->insert({param.first, param.second});
            }

            switch (stat->GetUnits())
            {
            case stats::Units::Complex:
                report.set_unit(SiteAgentReport::Units::SiteAgentReport_Units_Complex);
                break;
            case stats::Units::Count:
                report.set_unit(SiteAgentReport::Units::SiteAgentReport_Units_Count);
                break;
            case stats::Units::Milliseconds:
                report.set_unit(SiteAgentReport::Units::SiteAgentReport_Units_Milliseconds);
                break;
            case stats::Units::Decibels:
                report.set_unit(SiteAgentReport::Units::SiteAgentReport_Units_Decibels);
                break;
            case stats::Units::Hz:
                report.set_unit(SiteAgentReport::Units::SiteAgentReport_Units_Hz);
                break;
            case stats::Units::Percentage:
                report.set_unit(SiteAgentReport::Units::SiteAgentReport_Units_Percentage);
                break;
            case stats::Units::PicoSecondsPerSecond:
                report.set_unit(SiteAgentReport::Units::SiteAgentReport_Units_PicoSecondsPerSecond);
                break;
            } // switch units

            StatsReport(report);
        } // GetStatistics
    } // namespace stats
} // namespace cqp
