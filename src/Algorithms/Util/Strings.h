#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <iomanip>
#include "Algorithms/algorithms_export.h"

/// Support for "wide strings" has been removed with great prejudice
/// see http://utf8everywhere.org/ for rational

namespace cqp {

    /// @def CONSTSTRING
    /// Macro for defining a constant string
    #define CONSTSTRING constexpr const char*

    /// @def NAMEDSTRING(Name)
    /// Macro for defining a string who's value is the same as it's name
    /// @param Name The name of the string and it's value
    /// @return Name
    #define NAMEDSTRING(Name) \
        CONSTSTRING Name = #Name

    /// Concatenate the strings optionally separating them with a delimiter
    /// @param strings The strings to concatenate
    /// @param delimiter Separator between each string
    /// @return The concatenated string.
    ALGORITHMS_EXPORT std::string Join(const std::vector<std::string>& strings, const std::string& delimiter = "");

    /**
     * @brief str2int
     * Provides a hash of a string which can be used in switch statements
     * @param str
     * @param h
     * @return hash of str
     */
    constexpr size_t str2int(const char* str, int h = 0)
    {
        return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ static_cast<size_t>(str[h]);
    }

    /**
     * @brief StrCompI
     * Compare two string, ignoring case
     * @param left
     * @param right
     * @return true if they match
     */
    ALGORITHMS_EXPORT bool StrEqualI(const std::string& left, const std::string& right);

    /**
     * @brief ltrim
     * Remove white space from the left of the string
     * @note thanks to https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
     * trim from start (in place)
     * @param s string to trim
     */
    static inline void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
        {
            return !std::isspace(ch);
        }));
    }

    /**
     * @brief rtrim
     * Remove white space from the end of the string
     * @note thanks to https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
     * trim from start (in place)
     * @param s string to trim
     */
    static inline void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
        {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    /**
     * @brief trim
     * trim from both ends (in place)
     * @param s string to trim
     */
    static inline void trim(std::string &s)
    {
        ltrim(s);
        rtrim(s);
    }

    /**
     * @brief SplitString
     * Tokenise the string
     * @param[in] value string to tokenise
     * @param[out] dest tokens
     * @param[in] separator How to separate the string
     * @param[in] startAt Start point of value
     */
    ALGORITHMS_EXPORT void SplitString(const std::string& value, std::vector<std::string>& dest, const std::string& separator, size_t startAt = 0);


    /**
     * @brief SplitString
     * Tokenise the string
     * @param[in] value string to tokenise
     * @param[out] dest tokens
     * @param[in] separator How to separate the string
     * @param[in] startAt Start point of value
     */
    ALGORITHMS_EXPORT void SplitString(const std::string& value, std::unordered_set<std::string>& dest, const std::string& separator, size_t startAt = 0);

    /**
     * @brief ToDictionary
     * Convert a delimited string with key value pairs into a dictionary
     * @param delimited string to separate, eg ``key1=abc;key2=def``
     * @param pairSeparator separator between key value pairs
     * @param keyValueSep separator between key and value
     * @param dictionary destination for results
     */
    ALGORITHMS_EXPORT void ToDictionary(const std::string& delimited, std::map<std::string, std::string>& dictionary,
                                        char pairSeparator, char keyValueSep = '=');

    /**
     * @brief ToLower
     * Convert a string to lower case
     * @param mixed
     * @return mixed in lower case
     */
    inline std::string ToLower(const std::string& mixed)
    {
        std::string result(mixed);
        std::transform(mixed.begin(), mixed.end(), result.begin(), ::tolower);
        return result;
    }

    /**
     * @brief HexToBytes
     * Read a string formatted in hex as raw bytes
     * @param hex
     * @return byte array
     */
    ALGORITHMS_EXPORT std::vector<uint8_t> HexToBytes(const std::string& hex);

    /**
     * @brief ToHexString
     * @tparam T The type of value
     * @param value Value to convert to hex
     * @return The value in uppercase hex, at least 2 characters wide, no prefix
     */
    template<typename T>
    std::string ToHexString(const T& value)
    {
        using namespace std;
        std::stringstream result;

        result << setw(2) << setfill('0') << uppercase << hex << +value;
        return result.str();
    }

    /// Lookup table to go from a hex character to it's number
    struct CharToIntTable {
        /// storage for the table
        char tab[256] {0};

        /// constructor
      constexpr CharToIntTable() : tab {} {
        tab[0u + '1'] = 1;
        tab[0u + '2'] = 2;
        tab[0u + '3'] = 3;
        tab[0u + '4'] = 4;
        tab[0u + '5'] = 5;
        tab[0u + '6'] = 6;
        tab[0u + '7'] = 7;
        tab[0u + '8'] = 8;
        tab[0u + '9'] = 9;
        tab[0u + 'a'] = 10;
        tab[0u + 'A'] = 10;
        tab[0u + 'b'] = 11;
        tab[0u + 'B'] = 11;
        tab[0u + 'c'] = 12;
        tab[0u + 'C'] = 12;
        tab[0u + 'd'] = 13;
        tab[0u + 'D'] = 13;
        tab[0u + 'e'] = 14;
        tab[0u + 'E'] = 14;
        tab[0u + 'f'] = 15;
        tab[0u + 'F'] = 15;
      }
      /// accessor for looking up a value
      /// Usage: `escaped = charToIntTable[achar];`
      /// @param idx The value to look up
      /// @return mapped value
      constexpr char operator[](char const idx) const { return tab[static_cast<size_t>(idx)]; }

    }
    /// Lookup table to go from a hex character to it's number
    constexpr charToIntTable;


    /**
     * @brief CharFromHex
     * Take a hex string (00-FF) to it's integral value
     * @param hexString A 2 character string
     * @return the integral value of the hex string
     */
    inline char CharFromHex (
        const std::string& hexString
    )
    {
        char result {};
        if(hexString.length() >= 2)
        {
            // convert the hex character to a nibble
            const char upper = charToIntTable[hexString[0]];
            const char lower = charToIntTable[hexString[1]];
            // shift the upper nibble up and combine with the lower
            result = static_cast<char>(upper << 4) + lower;
        }

        return result;
    }

} // namespace cqp


