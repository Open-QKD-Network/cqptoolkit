/*!
* @file
* @brief URI
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 7/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <regex>

namespace cqp
{
    namespace net
    {
        struct IPAddress;
        struct SocketAddress;
    }

    /**
     * @brief The URI class
     * Stores and parses internet addresses like http://www.google.com:80
     */
    class CQPTOOLKIT_EXPORT URI
    {
    public:
        /**
         * @brief URI
         * Constructor
         */
        URI();
        /**
         * @brief URI
         * Constructor
         * @param input Initial value for the URI
         */
        URI(const std::string& input);
        /**
         * @brief URI
         * Constructor
         * @param input Initial value for the URI
         */
        URI(const char* input);
        /**
         * @brief URI
         * Constructor
         * @param socketAddress Socket address to construct URI from
         */
        URI(const net::SocketAddress& socketAddress);

        /**
         * @brief ToString
         * @return URI as a string
         */
        std::string ToString() const;
        /**
         * @brief operator std::string
         * @return URI as a string
         */
        operator std::string() const noexcept;
        /**
         * @brief operator net::SocketAddress
         * URI as a socket address
         */
        operator net::SocketAddress() const noexcept;

        /**
         * @brief Parse
         * @param input String value to conver to a URI
         * @return true on success
         */
        bool Parse(const std::string& input);

        /**
         * @brief GetScheme
         * @return The sheme section
         */
        std::string GetScheme() const;
        /**
         * @brief GetHost
         * @return The host section
         */
        std::string GetHost() const;

        /**
         * @brief GetHost
         * @return The host section
         */
        std::string GetHostAndPort() const;

        /**
         * @brief GetFragment
         * @return The Fragment section
         */
        std::string GetFragment() const;
        /**
         * @brief GetPort
         * @return The port section
         */
        uint16_t GetPort() const;

        /**
         * @brief GetPath
         * @return The path section
         */
        std::string GetPath() const;

        /**
         * @brief GetQueryParameters
         * @return All parameters
         */
        std::vector<std::pair<std::string, std::string>> GetQueryParameters() const;
        /**
         * @brief GetFirstParameter
         * Get the first instance of a parameter
         * @param key Key to search for
         * @param[out] value The value found
         * @param caseSensitive Whether to match the key using case or to ignore case
         * @return true if the key was found
         */
        bool GetFirstParameter(const std::string& key, std::string& value, bool caseSensitive = false) const;
        /// @copydoc GetFirstParameter
        bool GetFirstParameter(const std::string& key, uint8_t& value, bool caseSensitive = false) const;
        /// @copydoc GetFirstParameter
        bool GetFirstParameter(const std::string& key, size_t& value, bool caseSensitive = false) const;
        /// @copydoc GetFirstParameter
        bool GetFirstParameter(const std::string& key, long& value, bool caseSensitive = false) const;
        /// @copydoc GetFirstParameter
        bool GetFirstParameter(const std::string& key, bool& value, bool caseSensitive = false) const;
        /// @copydoc GetFirstParameter
        bool GetFirstParameter(const std::string& key, double& value, bool caseSensitive = false) const;

        /**
         * @brief operator []
         * Get a parameter by it's id
         * @param key The parameter id
         * @return The parameter value as a string
         */
        std::string operator [](const std::string& key) const noexcept;
        /**
         * @brief ResolveAddress
         * Try to use DNS to result the hostname
         * @param[out] destination The result of resolution
         * @return true if successful
         */
        bool ResolveAddress(net::IPAddress& destination) const noexcept;
        /// @copydoc ResolveAddress
        bool ResolveAddress(net::SocketAddress& destination) const noexcept;

        /**
         * @brief SetScheme
         * @param newValue new scheme section
         */
        void SetScheme(const std::string& newValue);
        /**
         * @brief SetHost
         * @param newValue new host value
         */
        void SetHost(const std::string& newValue);
        /**
         * @brief SetFragment
         * @param newValue new host value
         */
        void SetFragment(const std::string& newValue);
        /**
         * @brief SetPort
         * @param newValue
         */
        void SetPort(uint16_t newValue);
        /**
         * @brief SetPath
         * @param newValue
         */
        void SetPath(const std::string& newValue);

        /**
         * @brief SetPath
         * Set the path using key value pairs, seperated by sep
         * @param newPath elements of the path
         * @param sep seperator
         * @param encode Encode the path elements
         */
        void SetPath(const std::vector<std::string>& newPath, const std::string& sep, bool encode = true);

        /**
         * @brief SetParameter
         * Set/Overwrite the key value pair
         * @param key The key to use
         * @param value The value to set
         */
        void SetParameter(const std::string& key, const std::string& value);
        /**
         * @brief AddParameter
         * Add the key value pair to the parameters
         * @param key The key to use
         * @param value The value to set
         */
        void AddParameter(const std::string& key, const std::string& value);

        /**
         * @brief RemoveParameter
         * deletes all parameters with the specified key
         * @param key parameter to delete
         */
        void RemoveParameter(const std::string& key);

        /**
         * @brief operator ==
         * @param rhs
         * @return true if the URLs are identical
         */
        inline bool operator==(const URI& rhs) const
        {
            return scheme == rhs.scheme &&
                   host == rhs.host &&
                   port == rhs.port &&
                   path == rhs.path &&
                   parameters == rhs.parameters &&
                   fragment == rhs.fragment;
        }

        /**
         * @brief operator !=
         * @param rhs
         * @return true if the URLs are not identical
         */
        inline bool operator!=(const URI& rhs) const
        {
            return !(*this == rhs);
        }

        /**
         * @brief Encode
         * Make the string safe for use in urls by escaping nasty characters
         * @note this is done automatically by all the set functions
         * @param input unescaped string
         * @return safe string, read for use in urls
         */
        static std::string Encode(const std::string& input);

        /**
         * @brief Decode
         * Convert any encoded charaters back to thier standard unsafe values
         * @note this is done automatically by all the get functions
         * @param input escaped string
         * @return unsafe, unescaped string
         */
        static std::string Decode(const std::string& input) noexcept;

        /**
         * @brief ToDictionary
         * Splits elements of the uri into a dictionary. This is mainly intended to turn non-standard urls
         * like pkcs into somehting useful. Only the path and parameters are extracted, both are treated like
         * key-value pairs and stored in destination
         * @details
         * For example: pkcs:module=abc;id=123?some=thing => { { "module", "abc"}, {"id", "123"}, {"some", "thing"} }
         * @param destination
         * @param pathSeperator
         * @param keyValueSeperator
         */
        void ToDictionary(std::map<std::string, std::string>& destination, char pathSeperator = ';',
                          char keyValueSeperator = '=') const;

    protected:
        /// the first part of the uri (eg: http)
        std::string scheme;
        /// hostname (eg www.google.com)
        std::string host;
        /// port number after the hostname: (eg 80)
        uint16_t port = 0;
        // TODO: username
        /// Section after the first / after the scheme
        std::string path;
        /// key value pairs after the '?' sign, seperated by '&'
        std::vector<std::pair<std::string, std::string>> parameters;
        /// string appended to the path after the '#' sign
        std::string fragment;

        /// regular expression used to disect the string
        std::regex urlRegEx;
    };

} // namespace cqp


namespace std
{
    /**
     * Templated hash algorithm for arrays, allowing them to be used in unordred_map's
     */
    template<>
    struct hash<cqp::URI>
    {
        /**
         * @brief operator ()
         * @param a item to hash
         * @return hash of "a"
         */
        size_t operator()(const cqp::URI& a) const noexcept
        {
            return 31 + hash<string>()(a.ToString());
        }
    };

} // namespace std

