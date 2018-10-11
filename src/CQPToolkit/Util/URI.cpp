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
#include "Util.h"
#include "Util/Logger.h"
#include "CQPToolkit/Net/Socket.h"
#include "CQPToolkit/Util/Util.h"
#include "CQPToolkit/Net/DNS.h"
#include "curl/curl.h"

namespace cqp
{
    CURLcode LogCurlResult(CURLcode code)
    {
        if(code != CURLE_OK)
        {
            LOGERROR(curl_easy_strerror(code));
        }
        return code;
    }

    // compile the regular expression once
    // doing this at object creation is expensive
    const std::regex urlRegExTemplate(R"(^(([^:\/?#]+):)?(\/\/([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
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
            result += scheme + "://";
        }
        result += host;
        if(port != 0)
        {
            result += ":" + std::to_string(port);
        }
        if(!path.empty())
        {
            if(path[0] != '/')
            {
                result += "/";
            }
            result += path;
        }
        if(parameters.size() > 0)
        {
            bool addPrefix = false;
            result += "?";
            for(auto param : parameters)
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
                size_t colonPos = hostAndPort.find(":");

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
                    size_t equalPos = param.find("=");
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
        for(auto param : parameters)
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

    void URI::SetPath(const std::vector<std::string>& newPath, const std::string& sep)
    {
        bool addSep = false;
        path.clear();
        for(auto element : newPath)
        {
            if(addSep)
            {
                path += sep;
            }
            else
            {
                addSep = true;
            }
            path += element;
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

    std::string URI::Encode(const std::string& input)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        static CURL* curl = curl_easy_init();

        char* curlResult = curl_easy_escape(curl, input.c_str(), static_cast<int>(input.size()));
        std::string result {curlResult};
        curl_free(curlResult);
        return result;
    }

    std::string URI::Decode(const std::string& input) noexcept
    {
        std::string result;
        try
        {
            curl_global_init(CURL_GLOBAL_ALL);
            static CURL* curl = curl_easy_init();

            int curlResultLength = 0;
            char* curlResult = curl_easy_unescape(curl, input.c_str(), static_cast<int>(input.size()), &curlResultLength);
            result = std::string(curlResult, static_cast<size_t>(curlResultLength));
            curl_free(curlResult);
        }
        catch(const std::exception& e)
        {
            LOGERROR(e.what());
        }
        return result;
    }

    void URI::ToDictionary(std::map<std::string, std::string>& destination, char pathSeperator, char keyValueSeperator) const
    {
        cqp::ToDictionary(path, destination, pathSeperator, keyValueSeperator);
        for(auto element : parameters)
        {
            destination[element.first] = element.second;
        }
    }

    bool URI::GetFirstParameter(const std::string& key, std::string& value, bool caseSensitive) const
    {
        bool result = false;
        const std::string encodedKey(Encode(key));
        if(caseSensitive)
        {
            for(auto param : parameters)
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
            for(auto param : parameters)
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
