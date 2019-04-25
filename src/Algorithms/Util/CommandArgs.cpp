/*!
* @file
* @brief CommandArgs
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CommandArgs.h"
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Util/FileIO.h"
#include <fstream>

namespace cqp
{
    CommandArgs::~CommandArgs()
    {
        for(auto option : options)
        {
            delete option;
        }
        options.clear();
    }

    CommandArgs::Option& CommandArgs::AddOption(const std::string& longName, const std::string& shortName, const std::string& description)
    {
        Option* newOpt = new Option();
        newOpt->longName = longName;
        newOpt->shortName = shortName.substr(0, 1);
        newOpt->description = description;

        options.push_back(newOpt);
        if(!longName.empty())
        {
            if(longOptions.find(longName) == longOptions.end())
            {
                longOptions[longName] = newOpt;
            }
            else
            {
                LOGERROR("Command option already defined: " + longName);
            }
        }
        if(!shortName.empty())
        {
            if(shortOptions.find(shortName) == shortOptions.end())
            {
                shortOptions[shortName] = newOpt;
            }
            else
            {
                LOGERROR("Command option already defined: " + shortName);
            }
        }
        LOGTRACE("Option: " + longName + ", " + shortName + ", " + description);
        return *newOpt;
    }

    bool CommandArgs::Parse(const std::vector<std::string>& args)
    {
        bool result = true;
        using namespace std;

        cmdName = fs::BaseName(args[0]);

        // arg 0 is the name of the application
        for(auto argIt = args.begin() + 1; argIt != args.end(); argIt++)
        {
            if(argIt->size() > 2 && argIt->substr(0, 2) == "--")
            {
                // long arg
                size_t equalPos = argIt->find("=", 2);
                auto it = longOptions.find(argIt->substr(2, equalPos - 2));

                if(it != longOptions.end())
                {
                    it->second->set = true;
                    if(it->second->hasArgument)
                    {
                        if(equalPos != string::npos)
                        {
                            it->second->value = argIt->substr(equalPos + 1);
                        }
                        else
                        {
                            LOGERROR("Missing required argument for " + it->second->longName);
                            result = false;
                        }
                    }
                    else if (equalPos != string::npos)
                    {
                        LOGERROR(it->second->longName + " does not take a value: " + argIt->substr(equalPos + 1));
                        result = false;
                    }

                    if(!it->second->boundTo.empty())
                    {
                        properties[it->second->boundTo] = it->second->value;
                    }

                    if(it->second->callback)
                    {
                        it->second->callback(*it->second);
                    }
                }
                else
                {
                    LOGERROR("Unknown long option: " + *argIt);
                    result = false;
                }
            }
            else if(argIt->size() > 1 && (*argIt)[0] == '-')
            {
                auto nextArg = argIt + 1;
                auto it = shortOptions.find(argIt->substr(1));

                if(it != shortOptions.end())
                {
                    it->second->set = true;
                    if(it->second->hasArgument)
                    {
                        if(nextArg != args.end())
                        {
                            it->second->value = *nextArg;
                            argIt++;
                        }
                        else
                        {
                            LOGERROR(it->second->shortName + " missing argument.");
                            result = false;
                        }
                    }

                    if(!it->second->boundTo.empty())
                    {
                        properties[it->second->boundTo] = it->second->value;
                    }

                    if(it->second->callback)
                    {
                        it->second->callback(*it->second);
                    }
                }
                else
                {
                    LOGWARN("Unknown short option: " + *argIt);
                    result = false;
                }
            }
            else
            {
                LOGERROR("Invalid option: " + *argIt);
                result = false;
            }

            if(stopProcessing)
            {
                break; // for
            }
        }

        if(!stopProcessing)
        {
            for(auto opt : options)
            {
                if(opt->required && !opt->set)
                {
                    LOGERROR("Required argument missing: " + opt->shortName);
                    result = false;
                }
            }
        }

        return result;
    }

    void CommandArgs::StopOptionsProcessing()
    {
        stopProcessing = true;
    }

    void CommandArgs::PrintHelp(std::ostream& output, const std::string& header, const std::string& footer)
    {
        output << header << std::endl;

        output << "Usage: " << cmdName << " ";

        std::string shortOptionals;
        std::string required;
        std::string longOptoinals;

        for(auto arg : options)
        {
            if(!arg->required)
            {
                if(!arg->shortName.empty())
                {
                    shortOptionals += arg->shortName;
                }
                else
                {
                    longOptoinals += " [--" + arg->longName + "]";
                }
            }
            else
            {
                if(!arg->shortName.empty())
                {
                    required += " -" + arg->shortName;
                }
                else
                {
                    required += " [--" + arg->longName + "]";
                }
            }
        }

        output << required;
        if(!shortOptionals.empty())
        {
            output << " [-" << shortOptionals << "]";
        }

        output << longOptoinals;

        output << std::endl;
        output << std::endl;

        for(auto arg : options)
        {
            std::string line;
            line += "   ";
            if(!arg->shortName.empty())
            {
                line += "-" + arg->shortName;

                if(arg->hasArgument)
                {
                    line += " VALUE";
                }
            }

            if(!arg->shortName.empty() && !arg->longName.empty())
            {
                line += ", ";
            }

            if(!arg->longName.empty())
            {
                line += "--" + arg->longName;

                if(arg->hasArgument)
                {
                    line += "=VALUE";
                }
            }
            if(arg->required)
            {
                line += "   (REQUIRED)";
            }

            if(line.length() <= 6)
            {
                output << line << "  " << arg->description << std::endl;
            }
            else
            {
                output << line << std::endl;
                output << "       " << arg->description << std::endl;
            }
        } // for

        output << footer << std::endl;
    }

    std::string CommandArgs::PropertiesToString() const
    {
        std::string result;
        for(const auto& prop : properties)
        {
            result += prop.first + " = " + prop.second + "\n";
        }
        return result;
    }

    bool CommandArgs::LoadProperties(const std::string& filename)
    {
        bool result = false;
        std::ifstream inFile(filename, std::ios::in);

        if(inFile)
        {
            result = true;
            std::string line;
            while(getline(inFile, line))
            {
                size_t equalPos =line.find('=');
                std::string key = line.substr(0, equalPos);
                trim(key);
                if(equalPos == std::string::npos)
                {
                    properties[key] = "true";
                }
                else
                {
                    properties[key] = line.substr(equalPos + 1);
                    trim(properties[key]);
                }

            }
        }

        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, bool& out) const
    {
        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                if(StrEqualI(it->second, "true") || StrEqualI(it->second, "1") || StrEqualI(it->second, "yes"))
                {
                    out = true;
                    result = true;
                }
                else if(StrEqualI(it->second, "false") || StrEqualI(it->second, "0") || StrEqualI(it->second, "no"))
                {
                    out = false;
                    result = true;
                }
                else
                {
                    LOGWARN("Unknown boolean value: " + it->second);
                }
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, size_t& out) const
    {
        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                out = std::stoul(it->second);
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, int& out) const
    {
        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                out = std::stoi(it->second);
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, double& out) const
    {
        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                out = std::stod(it->second);
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, uint16_t& out) const
    {
        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                out = static_cast<uint16_t>(std::stoul(it->second));
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, uint32_t& out) const
    {
        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                out = static_cast<uint32_t>(std::stoul(it->second));
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, std::string& out) const
    {
        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                out = it->second;
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, PicoSeconds& out) const
    {
        using namespace std::chrono;

        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                if(it->second.length() > 2)
                {
                    // theres enough room for a suffix
                    const auto suffix = ToLower(it->second.substr(it->second.length() - 2, 2));
                    const auto value = it->second.substr(0, it->second.length() - 2);
                    if(suffix == "ms")
                    {
                        out = milliseconds(std::stoul(value));
                    }
                    else if(suffix == "ns")
                    {
                        out = nanoseconds(std::stoul(value));
                    }
                    else if(suffix == "ps")
                    {
                        out = PicoSeconds(std::stoul(value));
                    }
                    else
                    {
                        out = seconds(std::stoul(it->second));
                    }

                }
                else
                {
                    if(it->second.back() == 's' || it->second.back() == 'S')
                    {
                        out = seconds(std::stoul(it->second.substr(0, it->second.length() - 1)));
                    }
                    else
                    {
                        out = PicoSeconds(std::stoul(it->second));
                    }

                }
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, PicoSecondOffset& out) const
    {
        using namespace std::chrono;

        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                if(it->second.length() > 2)
                {
                    // theres enough room for a suffix
                    const auto suffix = ToLower(it->second.substr(it->second.length() - 2, 2));
                    const auto value = it->second.substr(0, it->second.length() - 2);
                    if(suffix == "ms")
                    {
                        out = milliseconds(std::stol(value));
                    }
                    else if(suffix == "ns")
                    {
                        out = nanoseconds(std::stol(value));
                    }
                    else if(suffix == "ps")
                    {
                        out = PicoSecondOffset(std::stol(value));
                    }
                    else
                    {
                        out = seconds(std::stol(it->second));
                    }

                }
                else
                {
                    if(it->second.back() == 's' || it->second.back() == 'S')
                    {
                        out = seconds(std::stol(it->second.substr(0, it->second.length() - 1)));
                    }
                    else
                    {
                        out = PicoSecondOffset(std::stol(it->second));
                    }

                }
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::GetProp(const std::string& key, AttoSecondOffset& out) const
    {
        using namespace std::chrono;

        bool result = false;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                if(it->second.length() > 2)
                {
                    // theres enough room for a suffix
                    const auto suffix = ToLower(it->second.substr(it->second.length() - 2, 2));
                    const auto value = it->second.substr(0, it->second.length() - 2);
                    if(suffix == "ms")
                    {
                        out = milliseconds(std::stol(value));
                    }
                    else if(suffix == "ns")
                    {
                        out = nanoseconds(std::stol(value));
                    }
                    else if(suffix == "ps")
                    {
                        out = PicoSecondOffset(std::stol(value));
                    }
                    else if(suffix == "fs")
                    {
                        out = FemtoSecondOffset(std::stol(value));
                    }
                    else if(suffix == "as")
                    {
                        out = AttoSecondOffset(std::stol(value));
                    }
                    else
                    {
                        out = seconds(std::stol(it->second));
                    }

                }
                else
                {
                    if(it->second.back() == 's' || it->second.back() == 'S')
                    {
                        out = seconds(std::stol(it->second.substr(0, it->second.length() - 1)));
                    }
                    else
                    {
                        out = PicoSecondOffset(std::stol(it->second));
                    }

                }
                result = true;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    std::string CommandArgs::GetStringProp(const std::string& key) const
    {
        std::string result;
        auto it = properties.find(key);
        if(it != properties.end())
        {
            try
            {
                result = it->second;
            }
            catch (const std::exception& e)
            {
                LOGERROR(e.what());
            }
        }
        return result;
    }

    bool CommandArgs::IsSet(const std::string& longName) const
    {
        bool result = false;
        auto it = longOptions.find(longName);
        if(it != longOptions.end())
        {
            result = it->second->set;
        }
        return result;
    }

    bool CommandArgs::HasProp(const std::string& key) const
    {
        return properties.find(key) != properties.end();
    } // PrintHelp

} // namespace cqp
