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
#pragma once
#include <string>
#include <array>
#include "Algorithms/algorithms_export.h"

namespace cqp
{
    /**
     * @brief The UUID class
     * Handler for globally unique identifiers
     */
    class ALGORITHMS_EXPORT UUID
    {
    public:
        /// Definition for the storage of a UUID
        using UUIDStorage = std::array<unsigned char, 16>;

        /**
         * @brief UUID
         * @return A guaranteed unique identifier
         */
        UUID();
        /**
         * @brief UUID
         * @param other
         * @return an id from a UUID string
         */
        UUID(const std::string &other);

        /**
         * @brief UUID
         * @param str Construct a uuid from the UTF8 hex string in the form of
         * XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
         */
        UUID(const char* str);

        /**
         * @brief UUID
         * @param values build a uuid from raw bytes stored in an array
         */
        UUID(const UUIDStorage& values);
        /**
         * @brief ToString
         * @return ID as a string
         */
        std::string ToString() const;
        /**
         * @brief operator std::string
         */
        operator std::string() const noexcept;


        /**
         * @brief IsValid
         * @return true if the uuid is not all 0
         */
        bool IsValid() const;

        /**
         * @brief IsValid
         * @param input UTF8 hex string in the form of
         * XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
         * @return true if the string representation is a valid uuid
         */
        static bool IsValid(const std::string& input);

        /**
         * @brief Null
         * @return a null id
         */
        static UUID Null();

        /**
         * @brief operator ==
         * @param other uuid to compare to
         * @return true if the uuids match
         */
        bool operator==(const UUID &other) const
        {
            return (value == other.value);
        }

        /// the bytes for the uuid
        UUIDStorage value {};
    };

} // namespace cqp

namespace std
{
    /**
     * Templated hash algorithm for arrays, allowing them to be used in unordered_map's
     */
    template<>
    struct hash<cqp::UUID>
    {
        /**
         * @brief operator ()
         * @param a item to hash
         * @return hash of "a"
         */
        size_t operator()(const cqp::UUID& a) const noexcept
        {
            hash<unsigned char> hasher;
            size_t h = 0;
            for (size_t i = 0; i < a.value.size(); ++i)
            {
                h = h * 31 + hasher(a.value[i]);
            }
            return h;
        }
    };

} // namespace std
