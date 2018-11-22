/*!
* @file
* @brief Stat
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <thread>
#include <condition_variable>
#include <set>
#include <atomic>
#include <queue>
#include "CQPToolkit/Util/Event.h"

namespace cqp
{
    namespace stats
    {
        /**
         * @brief The Units enum
         */
        enum Units
        {
            /// doesn't hold any value but groups other statistics
            Complex,
            /// An absolute value
            Count,
            /// Time in milliseconds
            Milliseconds,
            /// Relative value
            Percentage,
            /// logarithmic ratio
            Decibels,
            /// Frequency
            Hz
        };

        class ProcessingWorker;

        /**
         * @brief The StatBase class
         * Basis of all statistics
         */
        class CQPTOOLKIT_EXPORT StatBase
        {
        public:
            /**
             * @brief StatBase
             * Constructor
             * @param pathin name of the statistic
             * @param k The units
             */
            StatBase(std::vector<std::string> const & pathin, Units k = Units::Complex);

            /// Destructor
            virtual ~StatBase() = default;

            virtual void ProcessStats() = 0;

            /**
             * @brief GetRate
             * @return The number of updates per second
             */
            double GetRate() const;

            /**
             * @brief GetUnits
             * @return the units
             */
            Units GetUnits() const;
            /**
             * @brief GetUpdated
             * @return The time last updated
             */
            std::chrono::high_resolution_clock::time_point GetUpdated() const;

            /**
             * @brief GetId
             * @return a unique id for this stat
             */
            size_t GetId() const;
            /**
             * @brief GetPath
             * @return the full name of this stat
             */
            const std::vector<std::string>& GetPath() const;
            /**
             * @brief Reset
             * Clear all values
             */
            virtual void Reset();

            /**
             * @brief parameters
             * key,value pairs associated with this stat
             */
            std::unordered_map<std::string, std::string> parameters;

            void StopProcessingThread();

        protected:

            /// control access to incoming
            std::mutex incommingMutex;
            /// The descriptive name of the stat
            const std::vector<std::string> path;
            /// The type of data shown
            const Units units;
            /// id for this stat
            const size_t uniqueId;
            /// true if any value has been processed by DoWork
            bool modified {false};
            /// time last updated
            std::chrono::high_resolution_clock::time_point updated;
            /// number of updates per second
            double rate = {};

            /**
             * @brief Counter
             * @return a unique number for this type of stat
             */
            static size_t Counter();

            std::shared_ptr<ProcessingWorker> worker;
        };

        /**
         * @brief The ProcessingWorker class
         * Processes incoming stats
         */
        class CQPTOOLKIT_EXPORT ProcessingWorker
        {
        public:
            /**
             * @brief Instance
             * @return The single instance of this class
             */
            static std::shared_ptr<ProcessingWorker> Instance();

            /**
             * @brief Enque
             * Request a stat is processed by the worker
             * @param me Stat to process
             */
            void Enque(StatBase* me);

            /// Destructor
            ~ProcessingWorker();
        private:
            /// Constructor
            ProcessingWorker();

            /**
             * @brief Worker
             * Process the waiting stats
             */
            void Worker();

            /// Time before the DoWork thread stops waiting for new values
            ///  and checks if the thread should quit
            static const std::chrono::milliseconds timeout;
            std::thread processingThread;
            std::condition_variable processCv;
            std::mutex processMutex;
            using ObjectList = std::set<StatBase*>;
            ObjectList waitingObjects;
            std::atomic_bool processorReady{ false };
            bool stopProcessing = false;
        private:

            static std::weak_ptr<ProcessingWorker> me;
        };

        /// Forward declaration of Stat
        template<typename T>
        class CQPTOOLKIT_EXPORT Stat;

        /**
         * Callback interface for receiving stats
         * @tparam T Datatype which the stat will store
         */
        template<typename T>
        class CQPTOOLKIT_EXPORT IStatCallback
        {
        public:
            /**
             * @brief StatUpdated
             * Notification that a stat has been updated
             * @param stat The stat which has been updated
             */
            virtual void StatUpdated(const Stat<T>* stat) = 0;
            /// Destructor
            virtual ~IStatCallback() {}
        };

        /**
         * @brief The IAllStatsCallback class
         * Callback interface for receiving all datatypes
         */
        class CQPTOOLKIT_EXPORT IAllStatsCallback :
            public virtual IStatCallback<double>,
            public virtual IStatCallback<long>,
            public virtual IStatCallback<size_t>
        {

        };

        /// Definition of a statistic
        template<typename T>
        class CQPTOOLKIT_EXPORT Stat : public StatBase,
            public Event<void (IStatCallback<T>::*)(const Stat<T>*), &IStatCallback<T>::StatUpdated>
        {
        public:

            /**
             * @brief Stat
             * Construct a stat
             * @param pathin Name of the stat
             * @param k Kind of units
             */
            Stat(std::vector<std::string> const & pathin, Units k = Units::Complex) :
                StatBase(pathin, k)
            {
            }

            /// Destructor
            virtual ~Stat() = default;

            /**
             * @brief GetLatest
             * @return Latest value
             */
            T GetLatest() const
            {
                return latest;
            }
            /**
             * @brief GetAverage
             * @return Average value
             */
            T GetAverage() const
            {
                return average;
            }
            /**
             * @brief GetTotal
             * @return Total value
             */
            T GetTotal() const
            {
                return total;
            }

            /**
             * @brief GetMin
             * @return Minimum value
             */
            T GetMin() const
            {
                return min;
            }
            /**
             * @brief GetMax
             * @return Maximum value
             */
            T GetMax() const
            {
                return max;
            }

            /**
             * @brief Update
             * Store a new statistic value
             * @note It is safe to call this in time sensitive regions as the call is dispatched to a worker task
             *
             * @param value
             */
            void Update(T value)
            {
                /*lock scope*/
                {
                    std::lock_guard<std::mutex> lock(incommingMutex);
                    incommingValues.push(value);
                } /*lock scope*/

                worker->Enque(this);
            }

            /**
             * @brief DoWork
             * Process incoming stats and pass them to the listeners
             */
            void ProcessStats() override
            {
                using std::unique_lock;
                using std::condition_variable;
                using std::chrono::high_resolution_clock;

                // wait for a new stat to be added to the queue
                unique_lock<std::mutex> lock(incommingMutex);

                while(!incommingValues.empty())
                {
                    T value = incommingValues.front();
                    incommingValues.pop();
                    // unlock the incoming queue
                    lock.unlock();

                    auto timeNow = high_resolution_clock::now();
                    if(modified)
                    {
                        // calculate the different values
                        min = std::min(min, value);
                        max = std::max(max, value);
                        // get the duration as seconds, stored in a double
                        auto timeBetweenUpdates = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                      timeNow - updated);
                        rate = static_cast<double>(value) / static_cast<double>(timeBetweenUpdates.count()) / 1000.0;
                        average = (average + value) / 2;
                    }
                    else
                    {
                        // this is the first ever value, reset the calculated fields
                        min = value;
                        max = value;
                        average = value;
                    }

                    total += value;
                    latest = value;
                    updated = timeNow;
                    modified = true;

                    // notify the listeners
                    this->Emit(this);

                    // re-lock to check if there are any waiting values
                    lock.lock();
                } // while
            }

            /**
             * Store a new time based statistic value
             * @see http://en.cppreference.com/w/cpp/chrono/duration
             * @tparam Rep Repetition value
             * @tparam Period The fixed-point precision multiplier
             * @param duration New value to store
             */
            template<class Rep, class Period>
            void Update(std::chrono::duration<Rep, Period> duration)
            {
                using namespace std::chrono;
                // cast to internal storage type
                T simpleValue = duration_cast<std::chrono::duration<T, std::milli>>(duration).count();
                Update(simpleValue);
            }

            /**
             * @brief Reset
             * Clear all values
             */
            void Reset() override
            {
                StatBase::Reset();

                latest = {};
                average = {};
                total = {};
                min = {};
                max = {};
            }

        protected:
            /// Queue of stats to be processed
            std::queue<T> incommingValues;
            /// The last value processed by the DoWork Thread
            T latest = {};
            /// The running average processed by the DoWork Thread
            T average = {};
            /// The total value processed by the DoWork Thread
            T total = {};
            /// The minimum value processed by the DoWork Thread
            T min = {};
            /// The maximum value processed by the DoWork Thread
            T max = {};
        }; // Stat

    } // namespace stats
} // namespace cqp


