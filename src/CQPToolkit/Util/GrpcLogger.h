/*!
* @file
* @brief CQP Toolkit - GrpcLogger
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 06 Feb 2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Logging/Logger.h"
#include <grpc++/grpc++.h>
#include <google/protobuf/stubs/status.h>
#include "CQPToolkit/cqptoolkit_export.h"

namespace cqp
{
    /**
     * @brief StatusToString
     * @param status
     * @return Status as a string
     */
    CQPTOOLKIT_EXPORT std::string StatusToString(const grpc::Status& status);

    /**
     * @brief LogStatus
     * Logs any error that may occur
     * @param status The status to check
     * @param extraMessage Any other information to include if there is an error
     * @return The status passed in
     */
    CQPTOOLKIT_EXPORT grpc::Status LogStatus(const grpc::Status& status, const std::string& extraMessage = "");

    /// @copydoc LogStatus
    CQPTOOLKIT_EXPORT google::protobuf::util::Status LogStatus(const google::protobuf::util::Status& status, const std::string& extraMessage = "");
} // namespace cqp
