/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22/8/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <queue>
#include <deque>
#include <condition_variable>

namespace cqp
{
    /**
     * @brief The ConcurrentQueue class
     * thread save queue class with blocking pop and push
     * @tparam T The storage data type
     * @tparam _Sequence The storage class which holds T
     */
    template <typename T, typename _Sequence = std::deque<T> >
    class ConcurrentQueue : protected std::queue<T, _Sequence>
    {
        /// Type for the queue
        using TheQueue = std::queue<T, _Sequence>;

    public:
        /**
         * @brief pop
         * Remove an entry from the queue.
         * @param timeout the duration to wait before failing if the queue is empty
         * @param[out] out The data pulled from the queue, unchanged if nothing is removed from the queue
         * @return true on success
         */
        bool pop(const std::chrono::microseconds& timeout, T& out)
        {
            // get a lock on the mutex
            std::unique_lock<std::mutex> lock(changeMutex);

            // wait until the queue has something in it
            bool waitResult = dataAvailable.wait_for(lock, timeout, [&]()
            {
                return !TheQueue::empty();
            });

            if(waitResult)
            {
                // extract the value
                out = TheQueue::front();
                // remove the extracted item from the queue
                TheQueue::pop();
            }

            return waitResult;
        }

        /**
         * @brief pop
         * Remove an entry from the queue. This will block until something is available
         * @return the front item from the queue
         */
        T pop()
        {
            // get a lock on the mutex
            std::unique_lock<std::mutex> lock(changeMutex);

            // wait until the queue has something in it
            dataAvailable.wait(lock, [&]()
            {
                return !TheQueue::empty();
            });

            // extract the value
            T result = TheQueue::front();
            // remove the extracted item from the queue
            TheQueue::pop();
            return result;
        }

        /**
         * @brief push
         * Add an item to the queue
         * @param value data to add
         */
        void push(T& value)
        {
            // get a lock on the mutex
            std::unique_lock<std::mutex> lock(changeMutex);
            // add the data
            TheQueue::push(value);
            lock.unlock();
            // release any waiting thread
            dataAvailable.notify_one();
        }

        /// Default Constructor
        ConcurrentQueue()=default;
        /// Copy constructor (disabled)
        ConcurrentQueue(const ConcurrentQueue&) = delete;            // disable copying

        /**
         * @brief operator =
         * Assignment constructor (disabled)
         * @return the result object
         */
        ConcurrentQueue& operator=(const ConcurrentQueue&) = delete; // disable assignment
    protected:
        /// protect against multiple thread accessing the data
        std::mutex changeMutex;
        /// trigger for waiting for data to change
        std::condition_variable dataAvailable;
    };

}; // namespace cqp
