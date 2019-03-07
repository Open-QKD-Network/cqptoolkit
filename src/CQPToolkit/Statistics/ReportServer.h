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
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include "QKDInterfaces/IReporting.grpc.pb.h"
#include "Algorithms/Statistics/Stat.h"
#include <condition_variable>
#include "CQPToolkit/Interfaces/IQKDDevice.h"
#include "Algorithms/Util/Event.h"

namespace cqp
{
    namespace stats
    {

        using StatsPublisher = Event<void(IStatsReportCallback::*)(const remote::SiteAgentReport&), &IStatsReportCallback::StatsReport>;

        /**
         * @brief The ReportServer class
         * Receives statistics and emits reports to listeners
         */
        class CQPTOOLKIT_EXPORT ReportServer :
            public remote::IReporting::Service,
            public virtual stats::IAllStatsCallback,
            public StatsPublisher,
            public IStatsReportCallback
        {
        public:
            /**
             * @brief ReportServer
             *  Constructor
             */
            ReportServer() = default;

            virtual ~ReportServer() override;

            ///@{
            /// @name remote::IReporting interface

            /// Requests statistics from the server
            /// @param request The filter for message which are returned
            /// @param context
            /// @param writer Repeated messages containing the data requested.
            /// @return status
            grpc::Status GetStatistics(grpc::ServerContext* context,
                                       const remote::ReportingFilter* request,
                                       ::grpc::ServerWriter<remote::SiteAgentReport>* writer) override;

            ///@}

            ///@{
            /// @name  stats::IStatCallback<T> interface

            void StatUpdated(const stats::Stat<double>* stat) override;

            void StatUpdated(const stats::Stat<long>* stat) override;

            void StatUpdated(const stats::Stat<size_t>* stat) override;
            ///@}

            /**
             * queue up the stat to send it to listeners
             * @param report report to queue
             */
            void StatsReport(const remote::SiteAgentReport& report) override; // PublishStat

            /**
            * @brief SetAdditionalProperties
            * Set values to be sent with every report
            * @param addProps
            */
            void AddAdditionalProperties(const std::string& key, const std::string& value);
        protected: // members

            /**
             * @brief The Reportlistener struct
             * Stores the details of a listener
             */
            struct Reportlistener
            {
                /// how to filter out unwanted reports
                remote::ReportingFilter filter;
                /// time when the last report was sent to the listener
                std::chrono::high_resolution_clock::time_point lastUpdate;
                /// queued reports which have yet to be sent
                std::queue<remote::SiteAgentReport> reports;
                /// limits access to reports
                std::mutex reportMutex;
                /// notifies waiting thread that reports are available
                std::condition_variable reportCv;
            };
            /// All current listeners
            std::unordered_map<size_t, Reportlistener> remoteListeners;
            /// counter for giving each listener a unique id
            size_t nextListenerId = 0;
            /// protect access to list of listeners
            std::mutex listenersMutex;
            /// allows waiting for the listeners to leave
            std::condition_variable listenerCv;
            /// Properties to append to reports before they are sent
            KeyValue additional;
            /// should the thread exit
            bool shutdown = false;
        protected: // methods

            /**
             * @brief ShouldSendStat
             * Check wthether the listener is interested in this stat
             * @param filter The filter to check against
             * @param lastUpdate Time last changed
             * @param report The report being checked
             * @return true if the stat passes the filter
             */
            static bool ShouldSendStat(const remote::ReportingFilter* filter, const std::chrono::high_resolution_clock::time_point& lastUpdate, const remote::SiteAgentReport& report);

            /**
             * @brief CompleteReport
             * Fill in standard (non template) details
             * @param stat The data reported
             * @param report The message to fill in with other details
             */
            void CompleteReport(const stats::StatBase* stat, remote::SiteAgentReport& report);

        }; // ReportServer

    } // namespace stats
} // namespace cqp


