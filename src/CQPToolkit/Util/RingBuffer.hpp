/*!
* @file
* @brief CQP Toolkit - Ring Buffer
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <mutex>
#include <condition_variable>
#include <array>
#include <chrono>
#include "Platform.h"

namespace cqp
{
    /// A blocking buffer which has a fixed size. Callers are blocked until the action can be performed
    template<class T, size_t Max>
    class RingBuffer
    {
    public:

        /// Returns true if the buffer is full
        /// Non blocking
        /// @return true if the buffer is full
        bool IsFull()
        {
            return numElements == Max;
        }

        /// Add an item to the buffer, will block until a space is available or until tel_time timeout
        /// @param[in] in The data to store
        /// @param[in] rel_time The maximum time to wait for space to become available
        /// @return true if the item was sucessfull added to the buffer
        bool Push(const T& in, const std::chrono::microseconds& rel_time)
        {
            bool result = false;
            // Lock the continous variable so that it can be checked
            std::unique_lock<std::mutex> lock(fullCVLock);
            // Wait for space to add more elements
            bool spaceAvailable = fullCV.wait_for(lock, rel_time, [&]()
            {
                return (numElements < Max);
            });

            if(spaceAvailable)
            {
                {
                    // Get a lock on the buffer it's self
                    std::lock_guard<std::mutex> lock(changeMutex);
                    // Store the data
                    buffer[head] = in;
                    // Move the head pointer, acounting for wrapping
                    head++;
                    if (head >= Max)
                    {
                        head = 0;
                    }

                    numElements++;
                    result = true;
                }
                // If anything is waiting for the buffer to become non-empty, this will wake them up.
                emptyCV.notify_one();
            }

            return result;
        }

        /// Push data onto the buffer.
        /// This will block if the buffer is full.
        /// @param[in] in the data to store
        void Push(const T& in)
        {
            using namespace std::chrono;
            // repeatly call push until it succeeds
            while(!Push(in, microseconds(10)));
        }

        /// Return a item from the buffer
        /// This will block until an element is available
        /// @return the data removed from the buffer
        T Pop()
        {
            // lock for the continous varuable
            std::unique_lock<std::mutex> lock(emptyCVLock);
            // Wait until there are some elements to extract
            emptyCV.wait(lock, [&]()
            {
                return numElements > 0;
            });

            T result;

            {
                // Lock the buffer
                std::lock_guard<std::mutex> lock(changeMutex);
                // extract the value
                result = buffer[tail];
                tail++;
                if (tail >= Max)
                {
                    tail = 0;
                }
                numElements--;
            }
            // Notify any waiting push calls that there is now room for one more.
            fullCV.notify_one();

            return result;
        }

    protected:
        /// The storage for the data
        std::array<T, Max> buffer;

        /// @cond
        std::mutex changeMutex;
        std::mutex emptyCVLock;
        std::mutex fullCVLock;
        std::condition_variable emptyCV;
        std::condition_variable fullCV;
        size_t head = 0;
        size_t tail = 0;
        size_t numElements = 0;
        /// @endcond
    };
}