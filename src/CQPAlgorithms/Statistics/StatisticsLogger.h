/*!
* @file
* @brief StatisticsLogger
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPAlgorithms/Statistics/Stat.h"
#include <iostream>
#include <sstream>

namespace cqp
{
    namespace stats
    {
        /**
         * @brief The StatisticsLogger class
         * Takes statistics and writes them to the console
         */
        class CQPALGORITHMS_EXPORT StatisticsLogger :
            public virtual stats::IAllStatsCallback
        {
        public:
            /**
             * @brief StatisticsLogger
             * Constructor
             */
            StatisticsLogger() = default;

            /**
             * Converts a stat into a string
             * @tparam T The value storage type
             * @param stat The Stat to convert
             * @return Human readable values for the stat
             */
            template<typename T>
            std::string GetValueString(const stats::Stat<T>* stat)
            {
                using namespace std;
                using std::chrono::high_resolution_clock;
                string report = " latest: " + to_string(stat->GetLatest()) + ",";
                report += " average: " + to_string(stat->GetAverage()) + ",";
                report += " min: " + to_string(stat->GetMin()) + ",";
                report += " max: " + to_string(stat->GetMax()) + ",";
                report += " total: " + to_string(stat->GetTotal()) + ",";
                report += " rate: " + to_string(stat->GetRate());
                //TODO
                //auto timespec = high_resolution_clock::to_time_t(stat->GetUpdated());
                //report += " updated: " + to_string(std::ctime(&timespec));
                return report;
            }

            ///@{
            /// @name IStatCallback interface
            /// @copydoc stats::IStatCallback<T>::StatUpdated

            template<typename T>
            void TStatUpdated(const stats::Stat<T>* stat)
            {
                using namespace std;

                if(outputEnabled != Destination::None)
                {
                    stringstream message;
                    message << "name: \"" << StatTree(stat) <<
                            "\", id: " << std::to_string(stat->GetId()) <<
                            ", " << StatUnit(stat->GetUnits()) <<
                            ", rate: " << stat->GetRate() <<
                            "," << GetValueString(stat);

                    for(auto param : stat->parameters)
                    {
                        message << ", " << param.first + "=" + param.second;
                    }

                    std::lock_guard<std::mutex> lock(outputLock);
                    switch(outputEnabled)
                    {
                    case Destination::LogInfo:
                        LOGINFO(message.str());
                        break;
                    case Destination::StdOut:
                        cout << message.str() << endl;
                        break;
                    case Destination::StdErr:
                        cerr << message.str() << endl;
                        break;
                    case Destination::None:
                        break;
                    }
                }
            }

            /// @copydoc stats::IStatCallback<T>::StatUpdated
            void StatUpdated(const stats::Stat<double>* stat) override
            {
                TStatUpdated(stat);
            }
            /// @copydoc stats::IStatCallback<T>::StatUpdated
            void StatUpdated(const stats::Stat<long>* stat) override
            {
                TStatUpdated(stat);
            }
            /// @copydoc stats::IStatCallback<T>::StatUpdated
            void StatUpdated(const stats::Stat<size_t>* stat) override
            {
                TStatUpdated(stat);
            }
            ///@}

            /// The output device
            enum Destination
            {
                None, LogInfo, StdOut, StdErr
            };

            /**
             * @brief SetOutput
             * @param enabled
             */
            void SetOutput(Destination enabled)
            {
                outputEnabled = enabled;
            }
        protected:

            /**
             * @brief StatTree
             * @param whichStat
             * @return The full scoped name of the stat
             */
            static std::string StatTree(const stats::StatBase* whichStat);

            /**
             * @brief StatUnit
             * @param unit
             * @return unit suffix
             */
            static std::string StatUnit(stats::Units unit);

            /// should the output be printed
            Destination outputEnabled = None;
            /// protection for the output destination
            std::mutex outputLock;
        };
    } // namespace stats
} // namespace cqp


