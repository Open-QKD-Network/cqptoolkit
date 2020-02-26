/*!
* @file
* @brief SiftBase
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Interfaces/ISiftedPublisher.h"
#include "Algorithms/Util/Provider.h"
#include "Algorithms/Datatypes/Qubits.h"
#include "CQPToolkit/Sift/Stats.h"
#include "QKDInterfaces/Qubits.pb.h"
#include "CQPToolkit/cqptoolkit_export.h"
#include "CQPToolkit/Interfaces/IRemoteComms.h"

namespace cqp
{
    namespace sift
    {

        /**
         * @brief The SiftBase class
         * Common code for sifting data which is inherently pre-aligned.
         * The reciever data is indexed so discards for undetected qubits
         * are performed during the sifting.
         */
        class CQPTOOLKIT_EXPORT SiftBase : public Provider<ISiftedCallback>,
            public virtual IRemoteComms
        {
        public:
            /**
             * @brief SiftBase
             * Constructor
             * @param mutex The thread control mutex
             * @param conditional The thread control condition variable
             */
            SiftBase(std::mutex& mutex, std::condition_variable& conditional);

            /**
             * @brief SetDiscardedIntensities
             * Set which intensities should be ignored
             * @param intensities which values should be disgarded
             */
            void SetDiscardedIntensities(std::set<Intensity> intensities);

            /// Statistics produced by this class
            Statistics stats;
        protected:
            bool PackQubit(Qubit qubit, size_t index, const cqp::remote::BasisAnswers& answers,
                           JaggedDataBlock& siftedData,
                           uint_least8_t& offset, JaggedDataBlock::value_type& byteBuffer);

            /// identifier for this instance
            std::string instance;
            /// Counter for the sequence number used with each publication of a block of qubits
            SequenceNumber siftedSequence = 0;
            /// which intensities should be ignored
            std::set<Intensity> discardedIntensities;
            // calculate the number of bits in the current system.
            static constexpr uint8_t bitsPerValue = sizeof(DataBlock::value_type) * CHAR_BIT;
        private:
            /// a mutex for use with collectedStatesCv
            std::mutex& statesMutex;
            /// used for waiting for new data to arrive
            std::condition_variable& statesCv;
        };

    } // namespace sift
} // namespace cqp


