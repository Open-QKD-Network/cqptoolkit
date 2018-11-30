/*!
* @file
* @brief ErrorCorrection
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "ErrorCorrection.h"
#include <cstddef>                             // for size_t
#include <ratio>                               // for ratio
#include "CQPToolkit/ErrorCorrection/Stats.h"  // for Stats
#include "CQPAlgorithms/Statistics/Stat.h"                   // for Stat
#include "CQPAlgorithms/Logging/Logger.h"                       // for LOGTRACE

namespace cqp
{
    namespace ec
    {

        void ErrorCorrection::PublishCorrected()
        {
            using namespace std::chrono;
            using std::chrono::high_resolution_clock;
            // record the start time
            high_resolution_clock::time_point timerStart = high_resolution_clock::now();

            // package data ready for next stage
            // TODO
            SequenceNumber id = 0;
            std::unique_ptr<DataBlock> corrected(new DataBlock);
            // publish the corrected data
            if(listener)
            {
                listener->OnCorrected(id, move(corrected));
            }

            // example stat publish
            stats.TimeTaken.Update(high_resolution_clock::now() - timerStart);
            stats.Errors.Update(0.0L);

        } // PublishCorrected

        void ErrorCorrection::OnSifted(const SequenceNumber id, std::unique_ptr<JaggedDataBlock> siftedData)
        {
            LOGTRACE("Sifted data received");
            // collect incoming data, notify listeners of new data
            // TODO
            //receivedDataCv.notify_one();
            // just pass it on
            std::unique_ptr<DataBlock> corrected(new DataBlock);
            std::copy(siftedData->begin(), siftedData->end(), corrected->begin());

            if(listener)
            {
                listener->OnCorrected(id, move(corrected));
            }
            ecSeqId++;
        } // OnSifted

        void ErrorCorrection::DoWork()
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
                    //SequenceNumber id = 0;
                    DataBlock parities;
                    //verifier->NotifyDataParity(id, parities);

                    PublishCorrected();
                } // if(dataReady)
            } // while(!ShouldStop())
        } // DoWork
    } // namespace ec
} // namespace cqp
