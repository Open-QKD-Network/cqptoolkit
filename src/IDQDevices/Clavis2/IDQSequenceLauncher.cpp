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
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Net/Sockets/Socket.h"
#include "Algorithms/Net/DNS.h"
#include "Algorithms/Util/FileIO.h"
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
    const std::string IDQSequenceLauncher::ProgramName = "QKDSequence";

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
                LOGWARN("Provided line attenuation is 0, resetting to default: " + std::to_string(lineAttenuation));
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

    bool IDQSequenceLauncher::WaitForKey()
    {
        using namespace std;
        unique_lock<mutex> lock(keyReadyMutex);
        // wait for an event
        keyReadyCv.wait(lock, [&]()
        {
            return keyAvailable || shutdown;
        });

        bool result = keyAvailable;
        // reset for next time
        keyAvailable = false;
        return result;
    }

    IDQSequenceLauncher::~IDQSequenceLauncher()
    {
        shutdown = true;

        proc.RequestTermination(true);
        keyReadyCv.notify_all();

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

        const std::string configFile("/var/idq/vectis.conf");
        const std::string pskFolder(fs::Parent(configFile));
        const std::string logFolder("/var/log/idq");

        if(psk.size() != 32)
        {
            LOGWARN("Initial shared key must be 32 bytes long, not " + std::to_string(psk.size()));
        }

        try
        {
            // make sure it's log folder exists
            if(!fs::Exists(logFolder))
            {
                fs::CreateDirectory(logFolder);
            }

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
                ofstream fs(configFile, ios::out | ios::trunc);
                if(fs.is_open())
                {

                    fs << "[devices]" << endl;
                    fs << "max_devices = 12" << endl;
                    fs << "[installation]" << endl;
                    fs << "initial_secret_key = " << hex << uppercase << noshowbase << setw(2) << setfill('0');
                    for(uint8_t byte : psk)
                    {
                        // cast to int to get stream to output it as hex
                        fs << static_cast<int>(byte);
                    }
                    fs << endl;
                    fs.close();
                }
                else
                {
                    LOGERROR("Failed to write to vectis file");
                }
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
        const regex visibilitySystem("VisibilityMeasurement: Visibility of the system: ([0-9.]+)%");
        const regex qberRx("ErrorCorrectionCascade: QBER: ([0-9.]+)");
        const regex keySize("PrivacyAmplification: Final key size: ([0-9]+)");
        const regex lineLength(R"raw(LineMeasurement: Line length:\s+([0-9.]+))raw"); // raw string starts after raw( and ends at )raw
        const regex secretKeyRate(".+PrivacyAmplification: Secret Key Rate = ([0-9.]+)");

        try
        {
            LOGDEBUG("Starting " + fs::GetCurrentPath() + "/id3100/" + ProgramName + " " + Join(args, " "));
            proc.Start(fs::GetCurrentPath() + "/id3100/" + ProgramName, args, nullptr, &stdOut, nullptr);

            try
            {

                while (proc.Running() && !shutdown)
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
                        try
                        {
                            smatch matchResult;
                            if(std::regex_match(line, matchResult, visibilitySystem))
                            {
                                const double vis = stod(matchResult[0].str());
                                stats.Visibility.Update(vis);
                            }
                            else if(std::regex_match(line, matchResult, qberRx))
                            {
                                const double qber = stod(matchResult[0].str());
                                stats.Qber.Update(qber);
                            }
                            else if(std::regex_match(line, matchResult, keySize))
                            {
                                const ulong keySize = stoul(matchResult[0].str());
                                stats.keySize.Update(keySize);
                            }
                            else if(std::regex_match(line, matchResult, lineLength))
                            {
                                const ulong linelength = stoul(matchResult[0].str());
                                stats.lineLength.Update(linelength);
                            }
                            else if(std::regex_match(line, matchResult, secretKeyRate))
                            {
                                const double keyRate = stod(matchResult[0].str()); // bits per second
                                stats.keyRate.Update(keyRate);

                                {
                                    /*lockscope*/
                                    // trigger the host to collect key from the device
                                    unique_lock<mutex> lock(keyReadyMutex);
                                    keyAvailable = true;
                                } /*lockscope*/
                                keyReadyCv.notify_one();
                            }
                        }
                        catch (const exception& e)
                        {
                            LOGERROR(e.what());
                        }

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

            LOGDEBUG("Waiting for " + ProgramName + " to exit...");
            int result = proc.WaitForExit();

            if(result != 0)
            {
                LOGERROR(ProgramName + " [" + ProgramName + "] exited with return code: " + to_string(result));
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
