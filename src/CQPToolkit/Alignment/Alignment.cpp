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
#include "Alignment.h"
#include <bits/stdint-uintn.h>                   // for uint64_t
#include <google/protobuf/empty.pb.h>            // for Empty
#include <grpcpp/impl/codegen/client_context.h>  // for ClientContext
#include <cstddef>                               // for size_t
#include <cstdint>                               // for uint64_t
#include <ratio>                                 // for ratio
#include <string>                                // for operator+, to_string
#include <unordered_map>                         // for unordered_map
#include <utility>                               // for pair
#include "QKDInterfaces/Qubits.pb.h"             // for QubitsByFrame
#include "Statistics/Stat.h"                     // for Stat
#include "Stats.h"                               // for Statistics
#include "Util/Logger.h"                         // for LOGTRACE, LOGERROR
#include "CQPToolkit/Util/OpenCLHelper.h"
#include <algorithm>


namespace cqp
{
    namespace align
    {
    } // namespace align
} // namespace cqp
