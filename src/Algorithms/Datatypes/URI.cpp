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
#include "URI.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Net/DNS.h"
#include "Algorithms/Net/Sockets/Socket.h"

namespace cqp
{
    constexpr char SpaceSeperator = '+';
    constexpr char EscapeChar = '%';

    // compile the regular expression once
    // doing this at object creation is expensive
    // see https://stackoverflow.com/questions/5620235/cpp-regular-expression-to-validate-url
    const std::regex URI::urlRegExTemplate(R"foo(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)foo",
                                      std::regex::extended
                                     );

    // copy the compiled regex to prevent side effected with multiple instances
    URI::URI() : urlRegEx(urlRegExTemplate)
    {

    }

    URI::URI(const std::string& input) : URI()
    {
        Parse(input);
    }

    URI::URI(const char* input) : URI()
    {
        Parse(input);
    }

    URI::URI(const net::SocketAddress& socketAddress) : URI()
    {
        host = Encode(socketAddress.ip.ToString());
        port = socketAddress.port;
    }

    std::string URI::ToString() const
    {
        // all values are escaped when set

        std::string result;
        if(!scheme.empty())
        {
            result += scheme + ":";
            if(!host.empty())
            {
                result += "//";
            }
        }

        result += host;

        if(port != 0)
        {
            result += ":" + std::to_string(port);
        }

        result += path;

        if(!parameters.empty())
        {
            bool addPrefix = false;
            result += "?";
            for(const auto& param : parameters)
            {
                if(addPrefix)
                {
                    result += "&";
                }
                else
                {
                    addPrefix = true;
                }
                result += param.first;
                if(!param.second.empty())
                {
                    result += "=" + param.second;
                }
            }
        }
        if(!fragment.empty())
        {
            result += "#" + fragment;
        }
        return result;
    }

    cqp::URI::operator std::string() const noexcept
    {
        return ToString();
    }

    bool IsNumber(const std::string& s)
    {
        return !s.empty() && std::find_if(s.begin(),
                                          s.end(), [](char c)
        {
            return !std::isdigit(c);
        }) == s.end();
    }

    bool URI::Parse(const std::string& input)
    {
        using namespace std;
        bool result = true;
        smatch matchResult;

        scheme.clear();
        host.clear();
        port = 0;
        fragment.clear();
        parameters.clear();
        path.clear();

        if(std::regex_match(input, matchResult, urlRegEx))
        {
            if(!matchResult[1].matched &&
                    !matchResult[2].matched && !matchResult[3].matched &&
                    !matchResult[4].matched && matchResult[5].matched)
            {
                // just a hostname
                host = matchResult[5].str();
            } // if just host
            else if(!matchResult[4].matched && matchResult[2].matched && matchResult[5].matched &&
                    IsNumber(matchResult[5].str()))
            {
                // hostname and port
                host = matchResult[2].str();
                port = static_cast<uint16_t>(stoul(matchResult[5].str()));
            } // if host and port
            else if(matchResult[2].matched && !matchResult[3].matched && !matchResult[4].matched && matchResult[5].matched)
            {
                // URI without "//"
                scheme = matchResult[2];
                path = matchResult[5].str();
            } // if strange uri
            else if(matchResult.size() >= 6)
            {
                // full uri
                scheme = matchResult[2];
                const string& hostAndPort = matchResult[4];
                size_t colonPos = hostAndPort.find(':');

                host = hostAndPort.substr(0, colonPos);
                if(colonPos != string::npos)
                {
                    try
                    {
                        port = static_cast<uint16_t>(stoul(hostAndPort.substr(colonPos + 1)));
                    }
                    catch (const exception &e)
                    {
                        LOGERROR(e.what());
                        result = false;
                    } // catch
                } // if has port

                path = matchResult[5].str();
            } // if full url

            if(matchResult.size() >= 7 && matchResult[7].matched)
            {
                stringstream inStream(matchResult[7].str());
                string param;
                while(std::getline(inStream, param, '&'))
                {
                    size_t equalPos = param.find('=');
                    std::string value;
                    if(equalPos != std::string::npos)
                    {
                        value = param.substr(equalPos + 1);
                    }
                    parameters.push_back({param.substr(0, equalPos), value});
                }
            } // if has parameters

            if(matchResult.size() >= 9 && matchResult[9].matched)
            {
                fragment = matchResult[9];
            } // if has fragment
        } // if match

        return result;
    } // parse

    std::string URI::GetScheme() const
    {
        return Decode(scheme);
    }

    std::string URI::GetHost() const
    {
        return Decode(host);
    }

    std::string URI::GetHostAndPort() const
    {
        return GetHost() + ":" + std::to_string(GetPort());
    }

    std::string URI::GetFragment() const
    {
        return Decode(fragment);
    }

    uint16_t URI::GetPort() const
    {
        return port;
    }

    std::string URI::GetPath() const
    {
        return Decode(path);
    }

    std::vector<std::pair<std::string, std::string> > URI::GetQueryParameters() const
    {
        return parameters;
    }

    cqp::URI::operator net::SocketAddress() const noexcept
    {
        net::SocketAddress result;
        ResolveAddress(result.ip);
        result.port = port;
        return result;
    }

    bool URI::GetFirstParameter(const std::string& key, uint8_t& value, bool caseSensitive) const
    {
        bool result = false;
        std::string strVal;
        if(GetFirstParameter(key, strVal, caseSensitive))
        {
            try
            {
                value = static_cast<uint8_t>(std::stoul(strVal));
                result = true;

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool URI::GetFirstParameter(const std::string& key, size_t& value, bool caseSensitive) const
    {
        bool result = false;
        std::string strVal;
        if(GetFirstParameter(key, strVal, caseSensitive))
        {
            try
            {
                value = static_cast<size_t>(std::stoul(strVal));
                result = true;

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool URI::GetFirstParameter(const std::string& key, long& value, bool caseSensitive) const
    {
        bool result = false;
        std::string strVal;
        if(GetFirstParameter(key, strVal, caseSensitive))
        {
            try
            {
                value = std::stol(strVal);
                result = true;

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool URI::GetFirstParameter(const std::string& key, bool& value, bool caseSensitive) const
    {
        bool result = false;
        std::string strVal;
        if(GetFirstParameter(key, strVal, caseSensitive))
        {
            if(StrEqualI(strVal, "true") || StrEqualI(strVal, "1") || StrEqualI(strVal, "yes"))
            {
                value = true;
                result = true;
            }
            else if(StrEqualI(strVal, "false") || StrEqualI(strVal, "0") || StrEqualI(strVal, "no"))
            {
                value = false;
                result = true;
            }
            else
            {
                LOGWARN("Unknown boolean value: " + strVal);
            }
        }
        return result;
    }

    bool URI::GetFirstParameter(const std::string& key, double& value, bool caseSensitive) const
    {
        bool result = false;
        std::string strVal;
        if(GetFirstParameter(key, strVal, caseSensitive))
        {
            try
            {
                value = std::stod(strVal);
                result = true;

            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    std::string URI::operator [](const std::string& key) const noexcept
    {
        std::string result;
        for(const auto& param : parameters)
        {
            if(StrEqualI(param.first, key))
            {
                result = Decode(param.second);
                break; // for
            }
        }
        return result;
    }

    bool URI::ResolveAddress(net::IPAddress& destination) const noexcept
    {
        return net::ResolveAddress(host, destination);
    }

    bool URI::ResolveAddress(net::SocketAddress& destination) const noexcept
    {
        destination.port = port;
        return net::ResolveAddress(host, destination.ip);
    }

    void URI::SetScheme(const std::string& newValue)
    {
        scheme = Encode(newValue);
    }

    void URI::SetHost(const std::string& newValue)
    {
        host = Encode(newValue);
    }

    void URI::SetFragment(const std::string& newValue)
    {
        fragment = Encode(newValue);
    }

    void URI::SetPort(uint16_t newValue)
    {
        port = newValue;
    }

    void URI::SetPath(const std::string& newValue)
    {
        path = Encode(newValue);
    }

    void URI::SetPath(const std::vector<std::string>& newPath, const std::string& sep, bool encode)
    {
        bool addSep = false;
        path.clear();
        for(const auto& element : newPath)
        {
            if(addSep)
            {
                path += sep;
            }
            else
            {
                addSep = true;
            }
            if(encode)
            {
                path += Encode(element);
            } else {
                path += element;
            }
        }
    }

    void URI::SetParameter(const std::string& key, const std::string& value)
    {
        bool found = false;
        const std::string encodedKey(Encode(key));
        for(auto param = parameters.begin(); param != parameters.end(); param++)
        {
            if(param->first == encodedKey)
            {
                param->second = Encode(value);
                found = true;
                break; // for
            }
        }

        if(!found)
        {
            parameters.push_back({encodedKey, value});
        }
    }

    void URI::AddParameter(const std::string& key, const std::string& value)
    {
        parameters.push_back({Encode(key), Encode(value)});
    }

    void URI::RemoveParameter(const std::string& key)
    {
        for(auto param = parameters.rbegin(); param != parameters.rend(); param++)
        {
            if(param->first == key)
            {
                parameters.erase(std::next(param).base());
            }
        }
    }

    const std::set<char> URI::Unresserved = {
        '0', '1', '2', '3', '4',
        '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e',
        'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o',
        'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E',
        'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O',
        'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        '-', '.', '_', '~'
    };

    std::string URI::Encode(const std::string& input)
    {
        std::string result;
        // result will be at least as big as the input
        result.reserve(input.size());

        if(!input.empty())
        {
            for(const auto& c : input)
            {
                if(Unresserved.find(c) != Unresserved.end())
                    /* just copy this */
                    result.push_back(c);
                else if(c == ' ')
                {
                    result.push_back(SpaceSeperator);
                } else
                {
                    result.append(EscapeChar + ToHexString(c));
                }
            }
        }

        return result;
    }

    std::string URI::Decode(const std::string& input) noexcept
    {
        std::string result;
        try
        {
            for(size_t index = 0; index < input.size(); index++)
            {
                if(input[index] == SpaceSeperator)
                {
                    result.push_back(' ');
                } else if(input[index] == EscapeChar && input.size() > (index + 2))
                {
                    // decode the escaped character
                    std::string code;
                    code.push_back(input[index + 1]);
                    code.push_back(input[index + 2]);
                    result.push_back(CharFromHex(code));
                    index += 2;
                } else {
                    // copy as is
                    result.push_back(input[index]);
                }
            }
        }
        catch(const std::exception& e)
        {
            LOGERROR(e.what());
        }
        return result;
    }

    void URI::ToDictionary(std::map<std::string, std::string>& destination, char pathSeperator, char keyValueSeperator) const
    {
        cqp::ToDictionary(Decode(path), destination, pathSeperator, keyValueSeperator);
        for(const auto& element : parameters)
        {
            destination[element.first] = Decode(element.second);
        }
    }

    bool URI::GetFirstParameter(const std::string& key, std::string& value, bool caseSensitive) const
    {
        bool result = false;
        const std::string encodedKey(Encode(key));
        if(caseSensitive)
        {
            for(const auto& param : parameters)
            {
                if(param.first == encodedKey)
                {
                    value = Decode(param.second);
                    result = true;
                    break; // for
                }
            }
        }
        else
        {
            for(const auto& param : parameters)
            {
                if(StrEqualI(param.first, key))
                {
                    value = Decode(param.second);
                    result = true;
                    break; // for
                }
            }
        }
        return result;
    }

} // namespace cqp
