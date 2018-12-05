/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18/4/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "TestUtils.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/Keys.h"
#include <chrono>
#include "Algorithms/Datatypes/URI.h"
#include "Algorithms/Datatypes/UUID.h"
#include "Algorithms/Util/Process.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Util/Application.h"
#include "Algorithms/Net/Sockets/Socket.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Util/Hash.h"

namespace cqp
{
    namespace tests
    {
        const char* sourceData = "Calculate a hash (aka message digest) of data";


#pragma pack(push, 1)
        static constexpr const uint8_t MaxKeyLength = 32;

        /// Type of the datagram
        enum class DatagramType : uint8_t
        {
            KeyRequest = 0,
            KeyResponse
        };

        /// Fields common to all kinds of datagrams.
        struct KeyHeader
        {
            /// Type of the datagram
            DatagramType        datagramType;
            /// Key identifier of the requested key. Higher 4 octets
            /// of the KeyID. When both, KeyIDhw and KeyIDlw,
            /// parameters are set to 0 - a new key is requested.
            uint32_t            keyIDhw;
            /// Key identifier of the requested key. Lower 4 octets
            /// of the KeyID. When both, KeyIDhw and KeyIDlw,
            /// parameters are set to 0 - a new key is requested
            uint32_t            keyIDlw;
            /// The length of the key (in octets) to be generated.
            /// The default value is 32 octets (256 bit key), which is
            /// the maximum allowed value.
            uint8_t             keyLength;
            /// A number used to couple request –response pair.
            /// This number should be copied into the
            /// KeyRequestID parameter in the corresponding Key
            /// Response datagram
            uint32_t            keyRequestID;

        };

        /// Datagram for requesting a key from the device
        struct KeyRequest
        {
            /// Fields common to all kinds of datagrams.
            KeyHeader           header;
            /// The identification of the requesting ENC unit. The
            /// ID is used to uniquely identify each ENC unit when
            /// more than one ENC unit is connected to one QKS.
            /// This value is a writable setting on ENC unit.
            uint8_t             requestingDeviceID;
            /// Reserved for future use.
            uint8_t             reserved[10];
            /// Used to detect errors after transmission of the
            /// datagram. It will be computed before transmission of
            /// the datagram and should be verified by the receiving
            /// QKS.
            uint32_t            crc32;

        };

        /// Possible error codes resulting from requesting a key
        enum class ErrorStatus : uint8_t
        {
            Success = 0,
            NoMoreKeys,
            KeyIDDoesntExist,
            WrongKeyLength,
            InvalidKeyRequest
        };

        /// Array for converting error code into string
        const char* ErrorStatusString[5] =
        {
            "Success",
            "No More Keys",
            "Key ID Doesn't Exist",
            "Wrong Key Length",
            "Invalid Key Request"
        };

        /// Defines the data which comes from clavis device
        struct KeyResponse
        {
            /// Fields common to all kinds of datagrams.
            KeyHeader           header;
            /// Indicates success or failure of the Key Response.
            ErrorStatus         errorStatus;
            /// The requested or new key. Only the number of
            /// octets specified in the KeyLength parameter is used
            /// as a key.
            uint8_t             key[MaxKeyLength];
            /// The identification of the requesting ENC unit. The
            /// ID is used to uniquely identify each ENC unit when
            /// more than one ENC unit is connected to one QKS.
            /// This value is a writable setting on ENC unit.
            uint8_t             requestingDeviceID;
            /// Reserved for future use.
            uint8_t             reserved[10];
            /// Used to detect errors after transmission of the
            /// datagram. It will be computed before transmission of
            /// the datagram and should be verified by the receiving
            /// QKS.
            uint32_t            crc32;

        };

#pragma pack(pop)
        /**
         * @test
         * @brief TEST
         */
        TEST(UtilsTest, CheckIDQCRC)
        {
            const KeyResponse testPacket =
            {
                {
                    /* datagramType(8) */ DatagramType::KeyResponse,
                    /* keyIDhw(32) */0,
                    /* keyIDlw(32) */0,
                    /* keyLength(8) */32,
                    /* keyRequestID(32) */ 0x02 // ba1b23a0
                },
                /* errorStatus(8) */ErrorStatus::InvalidKeyRequest,
                /* key(8*32) */{0},
                /* requestingDeviceID(8) */0x01,
                /* reserved(8*10) */{0},
                /* crc32(32) */ (uint32_t)600975631 //0x4c530bd5
            };
            //uint32_t internalResult = CRC32(&testPacket, sizeof(testPacket));
            //ASSERT_EQ((uint32_t)0x31112CD4, internalResult);
            ASSERT_EQ((uint32_t)58, sizeof(testPacket) - sizeof(testPacket.crc32));
            uint32_t result = CRCFddi(reinterpret_cast<const unsigned char*>(&testPacket),
                                      sizeof(testPacket) - sizeof(testPacket.crc32));
            ASSERT_EQ((uint32_t)600975631, result);
        }

        TEST(UtilsTest, IPAddressTest)
        {
            net::IPAddress ip;
            ip.ip4 = {1, 2, 3, 4};
            ASSERT_EQ(ip.ToString(), "1.2.3.4");

            ip.ip6 = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x42, 0x83, 0x29};
            ip.isIPv4 = false;
            ASSERT_EQ(ip.ToString(), "2001:0db8:0000:0000:0000:ff00:0042:8329");

        }

        TEST(UtilsTest, URI)
        {
            ASSERT_EQ(URI::Encode("#4ImzZT9!@HU!P#bx$Ls%5Mv"), "%234ImzZT9%21%40HU%21P%23bx%24Ls%255Mv");
            ASSERT_EQ(URI::Decode("%234ImzZT9%21%40HU%21P%23bx%24Ls%255Mv"), "#4ImzZT9!@HU!P#bx$Ls%5Mv");
            ASSERT_EQ(URI::Encode("^mLY3yGr8&Zrp Jr0Iyn$99x!"), "%5EmLY3yGr8%26Zrp+Jr0Iyn%2499x%21");
            ASSERT_EQ(URI::Decode("%5EmLY3yGr8%26Zrp+Jr0Iyn%2499x%21"), "^mLY3yGr8&Zrp Jr0Iyn$99x!");
            URI uri("localhost:8000");
            ASSERT_EQ(uri.GetHost(), "localhost");
            ASSERT_EQ(uri.GetPort(), 8000);
            ASSERT_EQ(uri.GetPath(), "");

            ASSERT_TRUE(uri.Parse("https://secure.flickr.com:80/search/?q=gardens&diameter=3.6&ct=20000"));
            ASSERT_EQ(uri.GetScheme(), "https");
            ASSERT_EQ(uri.GetHost(), "secure.flickr.com");
            ASSERT_EQ(uri.GetPath(), "/search/");
            ASSERT_EQ(uri.GetPort(), 80);

            std::string paramValue;
            ASSERT_TRUE(uri.GetFirstParameter("q", paramValue));
            ASSERT_EQ(paramValue, "gardens");

            double paramDoubleValue = 0.0;
            ASSERT_TRUE(uri.GetFirstParameter("diameter", paramDoubleValue));
            ASSERT_EQ(paramDoubleValue, 3.6);

            long paramLongValue = 0;
            ASSERT_TRUE(uri.GetFirstParameter("ct", paramLongValue));
            ASSERT_EQ(paramLongValue, 20000);

            net::IPAddress ip;
            ASSERT_TRUE(uri.ResolveAddress(ip));
            ASSERT_FALSE(ip.IsNull());

            uri.AddParameter("ct", "3");
            uri.RemoveParameter("ct");
            ASSERT_FALSE(uri.GetFirstParameter("ct", paramLongValue));

            ASSERT_TRUE(uri.Parse("pkcs11:module-name=libsofthsm2.so;token=My%20token%201?pin-value=1234"));
            ASSERT_EQ(uri.GetScheme(), "pkcs11");
            ASSERT_EQ(uri.GetPath(), "module-name=libsofthsm2.so;token=My token 1");

        }

        TEST(UtilsTest, Process)
        {
            Process proc;
            int stdOut = 0;
            ASSERT_TRUE(proc.Start("/bin/date", {"+%%"}, nullptr, &stdOut));
            ASSERT_EQ(proc.WaitForExit(), 0);
            char buff[1024] {};
            ::read(stdOut, buff, sizeof (buff));
            ASSERT_STREQ(buff, "%\n");
        }

        TEST(UtilsTest, ProcessParams)
        {
            ConsoleLogger::Enable();
            DefaultLogger().SetOutputLevel(LogLevel::Debug);
            Process proc;
            int stdOut = 0;
            std::string basenameProg = "/bin/basename";
            if(!fs::Exists(basenameProg))
            {
                basenameProg = "/usr/bin/basename";
            }
            ASSERT_TRUE(proc.Start(basenameProg, {"/some/where/over/the/rain/bow", "w"}, nullptr, &stdOut));
            char buff[1024] {};
            ssize_t bytesRead = ::read(stdOut, buff, sizeof (buff));
            LOGDEBUG("Read " + std::to_string(bytesRead) + " bytes: " + std::string(buff, bytesRead));
            ASSERT_EQ(bytesRead, 3); // bo + newline
            ASSERT_STREQ(buff, "bo\n");
            ASSERT_EQ(proc.WaitForExit(), 0);
        }

        TEST(UtilsTest, FileIO)
        {
            ASSERT_EQ(fs::Parent("/somewhere/over/here"), "/somewhere/over");
#if defined(__linux)
            const std::string folderThatExists = "/dev";
            const std::string globThatExists = "/dev/tty*";
#elif defined(WIN32)
#endif
            ASSERT_TRUE(fs::Exists(folderThatExists));
            std::string wd = fs::GetCurrentPath();
            ASSERT_NE(wd, "");
            ASSERT_TRUE(fs::IsDirectory(wd));
            wd = fs::MakeTemp(true);
            ASSERT_NE(wd, "");
            ASSERT_TRUE(fs::Exists(wd));
            ASSERT_TRUE(fs::CanWrite(wd));

            std::vector<std::string> files = fs::ListChildren(folderThatExists);
            ASSERT_GT(files.size(), 0);
            files = fs::FindGlob(globThatExists);
            ASSERT_GT(files.size(), 0);

            ASSERT_TRUE(fs::Delete(wd));
            ASSERT_FALSE(fs::IsDirectory(wd));
            ASSERT_FALSE(fs::Exists(wd));

        }

        TEST(UtilsTest, Hash)
        {
            ASSERT_EQ(FNV1aHash("HelloHashy"), FNV1aHash("HelloHashy"));
            ASSERT_NE(FNV1aHash("HelloHashy"), FNV1aHash("HelloHashx"));
        }

        TEST(UtilsTest, Environment)
        {
            ASSERT_NE(cqp::fs::GetApplicationName(), "");
            ASSERT_NE(fs::GetCurrentPath(), "");
            ASSERT_NE(fs::GetHomeFolder(), "");
            ASSERT_NE(fs::GetCurrentPath(), "");

            auto tempFile = fs::MakeTemp();
            ASSERT_NE(tempFile, "");
            ASSERT_TRUE(fs::Exists(tempFile));
            ASSERT_FALSE(fs::IsDirectory(tempFile));

            ASSERT_TRUE(fs::Delete(tempFile));
            ASSERT_FALSE(fs::Exists(tempFile));

            auto tempDir = fs::MakeTemp(true);
            ASSERT_NE(tempDir, "");
            ASSERT_TRUE(fs::Exists(tempDir));
            ASSERT_TRUE(fs::IsDirectory(tempDir));

            ASSERT_TRUE(fs::Delete(tempDir));
            ASSERT_FALSE(fs::Exists(tempDir));

        }
    } // namespace tests
} // namespace cqp
