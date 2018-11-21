/*!
* @file
* @brief IDQSequenceLauncher
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 27/3/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "IDQSequenceLauncher.h"
#include <thread>
#include "CQPToolkit/Util/Logger.h"
#include "CQPToolkit/Util/Util.h"
#include "CQPToolkit/Util/URI.h"
#include "CQPToolkit/Net/Socket.h"
#include "CQPToolkit/Util/FileIO.h"
#include <fstream>
#include <iomanip>
#if defined(__unix__)
    #include <unistd.h>
#endif

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4251)
    #pragma warning(disable:4200)
#endif
#include "libusb-1.0/libusb.h"

namespace cqp
{
    /// Name of the program to run which interacts with the Clavis2 devices
    const std::string QKDSequence = "QKDSequence";

    IDQSequenceLauncher::IDQSequenceLauncher(const DataBlock& initialPsk, const std::string& otherUnit, double lineAttenuation):
        alice(true)
    {
        using namespace std;

        CreateInitialPsk(initialPsk);
        std::vector<std::string> args;

        if(DeviceFound(Clavis2ProductIDBob))
        {

            if(lineAttenuation == 0.0)
            {
                lineAttenuation = 3.0;
                LOGWARN("Provided line attenuation is 0, reseting to default: " + std::to_string(lineAttenuation));
            }

            alice = false;
            args = {"--bob", "--line_attenuation", to_string(lineAttenuation)};
        }
        else if(DeviceFound(Clavis2ProductIDAlice))
        {
            alice = true;
            try
            {
                URI otherUnitURI(otherUnit);
                net::IPAddress otherIP;
                if(otherUnitURI.ResolveAddress(otherIP))
                {
                    args = {"--alice", "--ip_bob", otherIP.ToString()};
                }
                else
                {
                    LOGERROR("Could not resolve: " + otherUnit);
                }
            }
            catch (const exception& e)
            {
                LOGERROR(e.what());
            }
        }
        else
        {
            LOGERROR("No Clavis 2 devices found");
        }

        if(!args.empty())
        {
            procHandler = thread(&IDQSequenceLauncher::LaunchProc, this, args);
        }
    }

    IDQSequenceLauncher::DeviceType IDQSequenceLauncher::DeviceFound()
    {
        DeviceType result = DeviceType::None;

        if(DeviceFound(Clavis2ProductIDAlice))
        {
            result = DeviceType::Alice;
        }
        else if(DeviceFound(Clavis2ProductIDBob))
        {
            result = DeviceType::Bob;
        }
        return result;
    }

    bool IDQSequenceLauncher::DeviceFound(uint16_t devId)
    {
        bool result = false;
        // Initialize the library.
        // by passing null we are using the default context which is reference counted
        if (libusb_init(nullptr) == LIBUSB_SUCCESS)
        {
            libusb_device_handle* dev = libusb_open_device_with_vid_pid(nullptr, IDQVendorId, devId);
            if(dev != nullptr)
            {
                result = true;
                libusb_close(dev);
            }
        }
        else
        {
            LOGERROR("Failed to initialise libUSB.");
        }

        return result;
    }

    bool IDQSequenceLauncher::Running()
    {
        return proc.Running();
    }

    IDQSequenceLauncher::~IDQSequenceLauncher()
    {
        proc.RequestTermination(true);
        if(procHandler.joinable())
        {
            procHandler.join();
        }
    }

    bool IDQSequenceLauncher::CreateInitialPsk(const DataBlock& psk)
    {
        bool result = false;
        using namespace std;

        // This is hard coded into the program

        std::string configFile("/var/idq/vectis.conf");
        std::string pskFolder(fs::Parent(configFile));

        if(psk.size() != 32)
        {
            LOGWARN("Initial shared key must be 32 bytes long, not " + std::to_string(psk.size()));
        }

        try
        {
            if(!fs::Exists(pskFolder))
            {
                result = fs::CreateDirectory(pskFolder);
            }
            else
            {
                result = fs::IsDirectory(pskFolder);
            }

            if(result && fs::CanWrite(pskFolder))
            {
                ofstream fs(configFile);


                fs << "[installation]" << endl;
                fs << "initial_secret_key = " << hex << uppercase << noshowbase << setw(2) << setfill('0');
                for(uint8_t byte : psk)
                {
                    fs << byte;
                }
                fs << endl;
                fs.close();

            }
            else
            {
                LOGERROR("Failed to access directory: " + pskFolder);
            }
        }
        catch (const std::exception& e)
        {
            result = false;
            LOGERROR(e.what());
        }
        return result;
    }

    void IDQSequenceLauncher::LaunchProc(const std::vector<std::string>& args)
    {
        using namespace std;
        int stdOut = 0;

        const string lineInfo = "INFO";
        const string lineWarn = "WARN";
        const string lineError = "ERROR";

        try
        {
            LOGDEBUG("Starting " + fs::GetCurrentPath() + "/id3100/" + QKDSequence + " " + Join(args, " "));
            proc.Start(fs::GetCurrentPath() + "/id3100/" + QKDSequence, args, nullptr, &stdOut, nullptr);

            try
            {

                while (proc.Running())
                {
                    string line;
                    char lastChar = {};

                    do
                    {
                        if(::read(stdOut, &lastChar, 1) > 0 && lastChar != '\n')
                        {
                            line.append(1, lastChar);
                        }
                    }
                    while (lastChar != 0 && lastChar != '\n');

                    if(line.compare(0, lineInfo.length(), lineInfo) == 0)
                    {
                        LOGINFO(line);
                    }
                    else if(line.compare(0, lineWarn.length(), lineWarn) == 0)
                    {
                        LOGWARN(line);
                    }
                    else if(line.compare(0, lineError.length(), lineError) == 0)
                    {
                        LOGERROR(line);
                    }
                    else
                    {
                        LOGERROR("Unknown line: " + line);
                    }
                }
            }
            catch (const exception& e)
            {
                LOGERROR(e.what());
            }

            LOGDEBUG("Waiting for " + QKDSequence + " to exit...");
            int result = proc.WaitForExit();

            if(result != 0)
            {
                LOGERROR(QKDSequence + " [" + QKDSequence + "] exited with return code: " + to_string(result));
            }
            else
            {
                LOGDEBUG("Process ended normally.");
            }

        }
        catch (const exception& e)
        {
            LOGERROR(e.what());
        }
    }

}
