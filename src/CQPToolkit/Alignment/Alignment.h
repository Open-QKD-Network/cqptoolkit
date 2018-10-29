/*!
* @file
* @brief Alignment
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 4/7/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <memory>                              // for shared_ptr
#include <grpcpp/impl/codegen/status.h>                   // for Status
#include <chrono>                                         // for seconds
#include <condition_variable>                             // for condition_v...
#include <memory>                                         // for unique_ptr
#include <mutex>                                          // for mutex
#include "CQPToolkit/Alignment/Stats.h"                   // for Statistics
#include "CQPToolkit/Interfaces/IAlignmentPublisher.h"    // for IAlignmentC...
#include "CQPToolkit/Interfaces/IDetectionEventPublisher.h"  // for IPhotonEven...
#include "CQPToolkit/Util/Provider.h"                        // for Event, Even...
#include "CQPToolkit/Util/WorkerThread.h"                 // for WorkerThread
#include "CQPToolkit/Datatypes/Base.h"                               // for SequenceNumber
#include "CQPToolkit/Datatypes/DetectionReport.h"                    // for ProtocolDet...
#include "CQPToolkit/Datatypes/Qubits.h"                             // for QubitList
#include "QKDInterfaces/IAlignment.grpc.pb.h"             // for IAlignment

namespace google
{
    namespace protobuf
    {
        class Empty;
    }
}
namespace grpc
{
    class ServerContext;
    class ChannelInterface;
}

namespace cl
{
    class Context;
    class Kernel;
    class Buffer;
} // namespace cl

namespace cqp
{
    namespace remote
    {
        class QubitsByFrame;
    }

    namespace align
    {
        /**
         * @brief The Alignment class
         * A simple example of aligning time tags as they are received.
         */
        class CQPTOOLKIT_EXPORT Alignment : public Provider<IAlignmentCallback>
        {
        public:

            /// Statistics collected by this class
            Statistics stats;
        protected:

            /**
             * @brief Alignment
             * Constructor
             */
            Alignment() {}

            /// a mutex for use with receivedDataCv
            std::mutex receivedDataMutex;
            /// used for waiting for new data to arrive
            std::condition_variable receivedDataCv;

            /// How long to wait for new data before checking if the thread should be stopped
            const std::chrono::seconds threadTimeout {1};

            /// our alignement sequence counter
            SequenceNumber seq = 0;

        }; // Alignment

    } // namespace align
} // namespace cqp


