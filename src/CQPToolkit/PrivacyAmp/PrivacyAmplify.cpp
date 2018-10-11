/*!
* @file
* @brief PrivacyAmplify
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "PrivacyAmplify.h"
#include "Stats.h"

namespace cqp
{
    namespace privacy
    {

        PrivacyAmplify::PrivacyAmplify()
        {

        } // PrivacyAmplify

        void PrivacyAmplify::PublishPrivacyAmplify()
        {
            // package data ready for next stage
            using std::chrono::high_resolution_clock;
            high_resolution_clock::time_point timerStart = high_resolution_clock::now();
            std::unique_ptr<KeyList> keys(new KeyList());
            keys->push_back(PSK());

            // package data ready for next stage
            for(std::unique_ptr<DataBlock>& block : incommingData)
            {
                (*keys)[0].insert((*keys)[0].end(), block->begin(), block->end());
            }

            if(keys->size() == 0 || (*keys)[0].size() == 0)
            {
                LOGWARN("Empty key");
            }
            incommingData.clear();
            LOGTRACE("Publishing key");
            const auto numKeys = keys->size();
            if(listener)
            {
                listener->OnKeyGeneration(move(keys));
            }

            stats.timeTaken.Update(high_resolution_clock::now() - timerStart);
            stats.keysEmitted.Update(numKeys);
        } // PublishPrivacyAmplify

        void PrivacyAmplify::OnCorrected(const ValidatedBlockID blockId,
            std::unique_ptr<DataBlock> correctedData)
        {
            LOGTRACE("Corrected Data recieved.");
            // collect incomming data, notify listeners of new data
            // TODO
            incommingData.push_back(std::move(correctedData));
            PublishPrivacyAmplify();
            //receivedDataCv.notify_one();
        } // OnTimeTagReport

        void PrivacyAmplify::DoWork()
        {
            using namespace std;
            while(!ShouldStop())
            {
                unique_lock<mutex> lock(receivedDataMutex);
                bool dataReady = receivedDataCv.wait_for(lock, threadTimeout, [&]
                {
                    // check if data is ready
                    return false; // TODO
                });

                if(dataReady)
                {
                    // TODO
                    // Converse with the other side

                    PublishPrivacyAmplify();
                } // if(dataReady)
            } // while(!ShouldStop())
        } // DoWork
    } // namespace privacy
} // namespace cqp
