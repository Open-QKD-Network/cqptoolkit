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
#include "CQPToolkit/Util/GrpcLogger.h"

namespace cqp
{
    std::string StatusToString(const grpc::Status& status)
    {
        using grpc::StatusCode;
        std::string result;
        switch(status.error_code())
        {
        case StatusCode::ABORTED:
            result += "ABORTED";
            break;
        case StatusCode::ALREADY_EXISTS:
            result += "ALREADY_EXISTS";
            break;
        case StatusCode::CANCELLED:
            result += "CANCELLED";
            break;
        case StatusCode::DATA_LOSS:
            result += "DATA_LOSS";
            break;
        case StatusCode::DEADLINE_EXCEEDED:
            result += "DEADLINE_EXCEEDED";
            break;
        case StatusCode::DO_NOT_USE:
            result += "DO_NOT_USE";
            break;
        case StatusCode::FAILED_PRECONDITION:
            result += "FAILED_PRECONDITION";
            break;
        case StatusCode::INTERNAL:
            result += "INTERNAL";
            break;
        case StatusCode::INVALID_ARGUMENT:
            result += "INVALID_ARGUMENT";
            break;
        case StatusCode::NOT_FOUND:
            result += "NOT_FOUND";
            break;
        case StatusCode::OK:
            result += "OK";
            break;
        case StatusCode::OUT_OF_RANGE:
            result += "OUT_OF_RANGE";
            break;
        case StatusCode::PERMISSION_DENIED:
            result += "PERMISSION_DENIED";
            break;
        case StatusCode::RESOURCE_EXHAUSTED:
            result += "RESOURCE_EXHAUSTED";
            break;
        case StatusCode::UNAUTHENTICATED:
            result += "UNAUTHENTICATED";
            break;
        case StatusCode::UNAVAILABLE:
            result += "UNAVAILABLE";
            break;
        case StatusCode::UNIMPLEMENTED:
            result += "UNIMPLEMENTED";
            break;
        case StatusCode::UNKNOWN:
            result += "UNKNOWN";
            break;
        }
        return result;
    }

    std::string StatusToString(google::protobuf::util::Status code)
    {
        using google::protobuf::util::error::Code;
        std::string result;
        switch(code.error_code())
        {
        case Code::ABORTED:
            result += "ABORTED";
            break;
        case Code::ALREADY_EXISTS:
            result += "ALREADY_EXISTS";
            break;
        case Code::CANCELLED:
            result += "CANCELLED";
            break;
        case Code::DATA_LOSS:
            result += "DATA_LOSS";
            break;
        case Code::DEADLINE_EXCEEDED:
            result += "DEADLINE_EXCEEDED";
            break;
        case Code::FAILED_PRECONDITION:
            result += "FAILED_PRECONDITION";
            break;
        case Code::INTERNAL:
            result += "INTERNAL";
            break;
        case Code::INVALID_ARGUMENT:
            result += "INVALID_ARGUMENT";
            break;
        case Code::NOT_FOUND:
            result += "NOT_FOUND";
            break;
        case Code::OK:
            result += "OK";
            break;
        case Code::OUT_OF_RANGE:
            result += "OUT_OF_RANGE";
            break;
        case Code::PERMISSION_DENIED:
            result += "PERMISSION_DENIED";
            break;
        case Code::RESOURCE_EXHAUSTED:
            result += "RESOURCE_EXHAUSTED";
            break;
        case Code::UNAUTHENTICATED:
            result += "UNAUTHENTICATED";
            break;
        case Code::UNAVAILABLE:
            result += "UNAVAILABLE";
            break;
        case Code::UNIMPLEMENTED:
            result += "UNIMPLEMENTED";
            break;
        case Code::UNKNOWN:
            result += "UNKNOWN";
            break;
        }
        return result;
    }

    grpc::Status LogStatus(const grpc::Status& status, const std::string& extraMessage)
    {
        using grpc::StatusCode;
        if(!status.ok())
        {
            std::string message = "[" + StatusToString(status) + "] " + status.error_message() + " - " + extraMessage + " - " + status.error_details();
            LOGERROR(message);
        }

        return status;
    }

    google::protobuf::util::Status LogStatus(const google::protobuf::util::Status& status, const std::string& extraMessage)
    {
        using grpc::StatusCode;
        if(!status.ok())
        {
            std::string message = "[" + StatusToString(status) + "] " + status.error_message().ToString() + " - " + extraMessage;
            LOGERROR(message);
        }

        return status;
    }
}
