#include "Algorithms/Util/Strings.h"
#include "Algorithms/Logging/Logger.h"

namespace cqp
{

    std::string Join(const std::vector<std::string>& strings, const std::string& delimiter)
    {
        using namespace std;
        string result;
        bool useDelim = false;
        const bool haveDeleim = !delimiter.empty();

        for(const auto& element : strings)
        {
            if(haveDeleim)
            {
                if(useDelim)
                {
                    result.append(delimiter);
                }
                else
                {
                    useDelim = true;
                }
            }

            result.append(element);
        }

        return result;
    }

    bool StrEqualI(const std::string& left, const std::string& right)
    {
        bool result = left.size() == right.size();
        if(result)
        {
            for(unsigned i = 0; i < left.length(); i++)
            {
                if(std::tolower(left[i]) != std::tolower(right[i]))
                {
                    result = false;
                    break; // for
                }
            }
        }

        return result;
    }

    void SplitString(const std::string& value, std::vector<std::string>& dest, const std::string& seperator, size_t startAt)
    {
        using std::string;
        size_t from = startAt;
        size_t splitPoint = 0;
        splitPoint = value.find(seperator, from);
        while(splitPoint != string::npos)
        {
            string iface = value.substr(from, splitPoint - from);
            if(!iface.empty())
            {
                dest.push_back(iface);
            }
            from = splitPoint + 1;
            splitPoint = value.find(seperator, from);
        }

        if(from < value.size())
        {
            dest.push_back(value.substr(from));
        }
    }

    void SplitString(const std::string& value, std::unordered_set<std::string>& dest, const std::string& seperator, size_t startAt)
    {
        std::vector<std::string> temp;
        SplitString(value, temp, seperator, startAt);
        for(auto& element: temp)
        {
            dest.insert(element);
        }
    }

    std::vector<uint8_t> HexToBytes(const std::string& hex)
    {
        std::vector<uint8_t> bytes;
        if(hex.length() % 2 == 0)
        {
            bytes.reserve(hex.length() / 2);

            for (unsigned int i = 0; i < hex.length(); i += 2)
            {
                std::string byteString = hex.substr(i, 2);
                uint8_t byte = static_cast<uint8_t>(strtoul(byteString.c_str(), nullptr, 16));
                bytes.push_back(byte);
            }
        }
        else
        {
            LOGERROR("Invalid length for hex bytes");
        }
        return bytes;
    }

    void ToDictionary(const std::string& delimited, std::map<std::string, std::string>& dictionary, char pairSeperator, char keyValueSep)
    {
        using namespace std;
        stringstream input(delimited);
        string param;
        while(getline(input, param, pairSeperator))
        {
            size_t equalPos = param.find(keyValueSep);
            std::string value;
            if(equalPos != std::string::npos)
            {
                value = param.substr(equalPos + 1);
            }
            dictionary[param.substr(0, equalPos)] = value;
        }
    }


    std::string KeyToPKCS11(KeyID keyId, const std::string& destination)
    {
        URI pkcs11Url;
        pkcs11Url.SetScheme("pkcs11");
        std::vector<std::string> pathElements;
        pathElements.push_back("type=secret-key");
        // this = PKCS label, used for the destination - so that it can be searched
        pathElements.push_back("object=" + URI::Encode(destination));
        pathElements.push_back("id=0x" + ToHexString(keyId));
        pkcs11Url.SetPath(pathElements, ";");

        return pkcs11Url.ToString();
    }

    std::string ToHexString(const std::string& value)
    {
        using namespace std;
        std::stringstream result;

        result << setw(2) << setfill('0') << uppercase << hex;
        for(const auto c : value)
        {
            result << static_cast<uint8_t>(c);
        }
        return result.str();
    }

} // namespace cqp
