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
#elif defined(WIN32)
    #include <rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
#endif

namespace cqp
{

    cqp::UUID::operator std::string() const noexcept
    {
        return this->ToString();
    }

    UUID::UUID(const UUID::UUIDStorage& values)
    {
        value = values;
    }

    UUID UUID::Null()
    {
        UUIDStorage nullValues {};
        return {nullValues};
    }

	UUID::UUID(const char* str) :
		UUID(std::string(str))
	{
	}

#if defined(__unix__)
    std::string UUID::ToString() const
    {
        char result[UUID_STR_LEN] {0}; /* FlawFinder: ignore */
        // uuid null terminates the string
        uuid_unparse(value.data(), result);
        return result;
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

    bool UUID::IsValid() const
    {
        return uuid_is_null(value.data()) == 0;
    }

    bool UUID::IsValid(const std::string& input)
    {
        UUIDStorage temp;
        return uuid_parse(input.c_str(), temp.data()) == 0;
    }

#elif defined(WIN32)
    std::string UUID::ToString() const
    {
		RPC_CSTR resultChar = nullptr;
		const ::UUID *temp = reinterpret_cast<const ::UUID*>(value.data());
		// UuidToStringA null terminates the string
        UuidToString(temp, &resultChar);
		std::string result(reinterpret_cast<const char*>(resultChar));
		RpcStringFree(&resultChar);

        return result;
    }

    UUID::UUID()
    {
        UuidCreate(reinterpret_cast<::UUID*>(value.data()));
    }

    UUID::UUID(const std::string& other)
    {
		RPC_CSTR* temp = new RPC_CSTR[other.size() + 1];
		strcpy_s(reinterpret_cast<char*>(*temp), other.size(), other.c_str());

        UuidFromString(*temp, reinterpret_cast<::UUID*>(value.data()));
		delete[] temp;
    }

	bool UUID::IsValid() const
	{
		RPC_STATUS status;
		auto temp = value;
		return UuidIsNil(reinterpret_cast<::UUID*>(temp.data()), &status) == 0;
	}

    bool UUID::IsValid(const std::string& input)
    {
		UUID temp(input);
		return temp.IsValid();
    }
#endif
} // namespace cqp
