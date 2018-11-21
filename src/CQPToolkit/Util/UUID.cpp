/*!
* @file
* @brief UUID
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 9/11/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "UUID.h"
#if defined(__linux)
    #include "uuid/uuid.h"
#else
    #if defined(WIN32)
        #include <rpc.h>
    #endif
#endif

namespace cqp
{

#if defined(__unix__)
    std::string UUID::ToString() const
    {
        char result[UUID_STR_LEN] {0}; /* FlawFinder: ignore */
        // uuid null terminates the string
        uuid_unparse(value.data(), result);
        return result;
    }

    cqp::UUID::operator std::string() const noexcept
    {
        return this->ToString();
    }

    UUID::UUID()
    {
        uuid_generate(value.data());
    }

    UUID::UUID(const std::string& other)
    {
        if(other.length() > 0)
        {
            uuid_parse(other.c_str(), value.data());
        }
    }

    UUID::UUID(const UUID::UUIDStorage& values)
    {
        value = values;
    }

    bool UUID::IsValid() const
    {
        return uuid_is_null(value.data()) == 0;
    }

    bool UUID::IsValid(const std::string& input)
    {
        UUIDStorage temp;
        return uuid_parse(input.c_str(), temp.data()) == 0;
    }

    UUID UUID::Null()
    {
        UUIDStorage nullValues {};
        return UUID(nullValues);
    }

    UUID::UUID(const char* str)
    {
        std::string other(str);
        if(other.length() > 0)
        {
            uuid_parse(other.c_str(), value.data());
        }
    }

#else
#if defined(WIN32)
    std::string ToString(const cqp::UUID& input)
    {
        char result[UUID_STR_LEN] {0}; /* FlawFinder: ignore */
        // UuidToStringA null terminates the string
        UuidToStringA(input.data(), result);
        return result;
    }

    cqp::UUID NewUUID()
    {
        UUID result;
        UuidCreate(result.data());
        return result;
    }

    cqp::UUID ToUUID(const std::string& other)
    {
        UUID result;
        UuidFromString(other.c_str(), result.data());
        return result;
    }

    bool IsValid(const cqp::UUID& input)
    {
        return UuidIsNil(input.data()) == 0;
    }
#endif
#endif
} // namespace cqp
