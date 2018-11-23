/*!
* @file
* @brief CQP Toolkit - Key Converter
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27 Jul 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "CQPToolkit/Util/Provider.h"
#include "CQPToolkit/Datatypes/Keys.h"
#include <bitset>
#include <climits>
#pragma once

namespace cqp
{
    namespace keygen
    {
        /// Process class to turn raw bits into key and send them to listeners
        class CQPTOOLKIT_EXPORT KeyConverter :
            public virtual IKeyCallback,
            public Provider<IKeyCallback>
        {
        public:
            /**
             * @brief KeyConverter
             * Constructor
             * @param bytesPerKey
             */
            explicit KeyConverter(size_t bytesPerKey = 16);

            ///@{
            ///  IKeyCallback interface

            /**
             * @brief OnKeyGeneration
             * Combine existing and new bytes of data into a key
             * @param keyData
             */
            void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;
            ///@}
        protected:
            /// The bits may be used on future key generations
            std::bitset<sizeof(uintmax_t) * CHAR_BIT> spares;

            /// Storage for bytes carried over between calls to BuildKeyList
            DataBlock carryOverBytes;

            /// the expected size of the key
            const size_t bytesInKey;
        };
    } // namespace keygen
} // namespace cqp
