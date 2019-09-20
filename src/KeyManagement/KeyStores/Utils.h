/*!
* @file
* @brief Utils
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 20/9/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "KeyManagement/keymanagement_export.h"
#include "KeyManagement/KeyStores/IBackingStore.h"

namespace cqp
{
    namespace keygen
    {

        class KEYMANAGEMENT_EXPORT Utils
        {
        public:
            Utils();
            static bool PopulateRandom(const std::string& leftId, IBackingStore& leftStore,
                                       const std::string& rightId, IBackingStore& rightStore,
                                       uint64_t numberKeysToAdd, uint16_t keyBytes = 32);

            using KeyStores = std::vector<std::pair<std::string, std::shared_ptr<keygen::IBackingStore>>>;

            static bool PopulateRandom(const KeyStores& stores,
                                       uint64_t numberKeysToAdd, uint16_t keyBytes = 32);
        };

    } // namespace keygen
} // namespace cqp


