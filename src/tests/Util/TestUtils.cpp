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

        TEST(UtilsTest, MultibyteXor)
        {
            PSK left
            {
                0x56, 0x10, 0xdd, 0x28, 0x24, 0xf8, 0x31, 0xf0, 0xb8, 0xd4, 0x2e, 0xcc, 0x62, 0xbe, 0xfb, 0x0b,
                0x5a, 0xca, 0x29, 0x9a, 0x42, 0x5f, 0xdf, 0xc2, 0x9e, 0xb4, 0xa8, 0x59, 0x02, 0x34, 0x2c, 0x78,
                0x1b, 0xab, 0xa2, 0xc9, 0xd1, 0x42, 0x4b, 0x16, 0x28, 0xd1, 0x55, 0x61, 0x9c, 0xf9, 0x0c, 0x0a,
                0x66, 0xe9, 0xf3, 0x62, 0x44, 0x44, 0x57, 0xde, 0xcd, 0xb7, 0x8e, 0xe5, 0xd5, 0x06, 0xcc, 0x3b,
                0xe5, 0x9d, 0x46, 0x1c, 0xc1, 0x5c, 0x87, 0x12, 0xf0, 0x1f, 0x64, 0xd6, 0x6d, 0x0a, 0x55, 0x13,
                0xb2, 0xa4, 0x61, 0xe2, 0x6c, 0x2a, 0xe6, 0xbb, 0x09, 0xda, 0xd0, 0x37, 0x74, 0xd7, 0x11, 0xac,
                0xe7, 0x61, 0xf7, 0x6d, 0x85, 0x0d, 0xf7, 0xa9, 0x71, 0xd6, 0xc7, 0x80, 0xef, 0x69, 0x29, 0x3d,
                0xd6, 0x6c, 0xb8, 0xae, 0xa6, 0xab, 0xf9, 0x90, 0x79, 0x80, 0x71, 0xa4, 0x56, 0xde, 0xdf, 0x3b,
                0xae, 0xa6, 0x73, 0xd5, 0xfb, 0xf3, 0xa6, 0xc9, 0xed, 0x9e, 0x59, 0xbe, 0xe7, 0xaa, 0xd3, 0x28,
                0x7c, 0xfd, 0x90, 0xf2, 0xd2, 0x50, 0x91, 0x95, 0x3b, 0x5a, 0x77, 0x2b, 0xc0, 0x84, 0x87, 0x01,
                0x2c, 0x8d, 0xff, 0x8d, 0xb9, 0x4b, 0x4b, 0x06, 0xb9, 0x41, 0xb7, 0x8f, 0x2d, 0x7c, 0xc3, 0x9d,
                0x7a, 0x11, 0x1b, 0xc8, 0x69, 0x0b, 0x93, 0xbd, 0xf6, 0xdf, 0x94, 0xbe, 0x3b, 0xdf, 0x16, 0x5e,
                0x3e, 0x1b, 0xe2, 0xcc, 0x67, 0xeb, 0xc5, 0x98, 0x12, 0xd1, 0x13, 0x49, 0x57, 0x5f, 0x58, 0x4d,
                0xb8, 0xfc, 0xbd, 0xfb, 0x87, 0xb1, 0x78, 0x66, 0xa1, 0x7e, 0x44, 0x35, 0x03, 0x96, 0xe1, 0x58,
                0xe4, 0x54, 0x07, 0x0f, 0x5e, 0x11, 0xab, 0xea, 0x95, 0x0e, 0x06, 0x10, 0x19, 0x54, 0x6d, 0x4f,
                0x97, 0x16, 0xa5, 0x8e, 0xa5, 0x7c, 0xdf, 0x46, 0x41, 0x2e, 0x88, 0x63, 0xf0, 0x94, 0xb8, 0x14,
                0x63, 0xd9, 0x29, 0x3f, 0xb2, 0xae, 0xdb, 0xf5, 0xa8, 0x6d, 0xd9, 0x37, 0xc0, 0x03, 0xf3, 0xc6,
                0xbc, 0x99, 0xd5, 0xf0, 0x46, 0xe8, 0xd1, 0xc5, 0x32, 0xf4, 0x8f, 0x56, 0xe6, 0xea, 0x65, 0x74,
                0xdd, 0xf3, 0x3f, 0xf9, 0x81, 0xb4, 0x57, 0xb9, 0x6c, 0x6f, 0xbc, 0x32, 0x24, 0x3f, 0xa0, 0x8a,
                0xd7, 0x4a, 0x78, 0xbe, 0x98, 0x99, 0x5e, 0x86, 0x02, 0x0e, 0x68, 0x80, 0x93, 0xa5, 0xb1, 0x1f,
                0xd0, 0x41, 0x55, 0x99, 0x20, 0x04, 0xfd, 0x50, 0x2b, 0xf2, 0x3f, 0x21, 0x83, 0x0f, 0x04, 0xef,
                0x93, 0x58, 0xa8, 0x7c, 0xbf, 0xc1, 0x5a, 0x5c, 0x17, 0xca, 0x19, 0x20, 0x1d, 0xaa, 0xa5, 0xe2,
                0xb5, 0x4a, 0xba, 0xda, 0x4b, 0x00, 0x57, 0xbb, 0x4b, 0x09, 0x54, 0xfa, 0xbb, 0x43, 0xaf, 0xb4,
                0x92, 0xd2, 0xf8, 0x54, 0x13, 0x98, 0x60, 0x06, 0xe2, 0xd6, 0x93, 0xa1, 0x3d, 0x55, 0x34, 0x1a,
                0x9a, 0x98, 0xbc, 0xe0, 0xe6, 0x66, 0x49, 0x32, 0xed, 0x40, 0x2f, 0x5f, 0x53, 0x0d, 0xb7, 0xc2,
                0x48, 0x5c, 0x7a, 0x4b, 0x7b, 0xb6, 0x9c, 0xec, 0x48, 0xcc, 0xce, 0xa5, 0x5b, 0xf6, 0xa6, 0x63,
                0x42, 0x99, 0xcd, 0xb7, 0x89, 0x1a, 0x30, 0x21, 0x8e, 0x08, 0x0d, 0xab, 0x14, 0x7b, 0x55, 0x1d,
                0x45, 0xcb, 0x63, 0xe8, 0x9f, 0xb1, 0x77, 0xcd, 0x2e, 0x3e, 0x57, 0xfa, 0x8c, 0x8a, 0x63, 0x8e,
                0x34, 0x0c, 0x04, 0xb9, 0x07, 0xf7, 0xa1, 0x19, 0x26, 0x53, 0xbb, 0xb3, 0x54, 0xb8, 0xdc, 0xe3,
                0xee, 0xb4, 0x74, 0x04, 0xb5, 0x9c, 0xd1, 0xcd, 0x61, 0xc7, 0x64, 0x9a, 0xf3, 0x48, 0x29, 0xde,
                0x97, 0x2f, 0x5c, 0x15, 0x05, 0x42, 0xe4, 0x8a, 0x33, 0x9e, 0xb5, 0x1d, 0x4c, 0xca, 0x02, 0x04,
                0xe4, 0x15, 0x12, 0x98, 0xc2, 0x7e, 0x87, 0x82, 0xfb, 0x51, 0x4f, 0xdb, 0x5e, 0x50, 0x7f, 0xa8,
                0xf6, 0xa7, 0xc1, 0xb5, 0xdf, 0x58, 0xf9, 0xc7, 0x94, 0x9a, 0xf0, 0xe7, 0x82, 0x98, 0xb1, 0xc8,
                0x66, 0xa5, 0x7a, 0xf6, 0xb4, 0xd0, 0xa3, 0xc3, 0xdf, 0x50, 0xca, 0x6d, 0x5d, 0x31, 0xe1, 0x69,
                0xee, 0xa8, 0xb9, 0x00, 0x58, 0xe9, 0x63, 0x69, 0x00, 0x8b, 0x2b, 0x21, 0x04, 0xf1, 0x95, 0x4c,
                0xa2, 0x34, 0xad, 0x6c, 0x4d, 0x1c, 0xc9, 0x5d, 0xfb, 0xf3, 0x35, 0xa0, 0x0e, 0x28, 0x8a, 0x7f,
                0x53, 0xc0, 0xda, 0x16, 0x31, 0x61, 0x93, 0xfa, 0xd1, 0x6e, 0xea, 0xa1, 0x8b, 0xd0, 0xae, 0xd7,
                0xa2, 0xad, 0x34, 0xc7, 0x32, 0xee, 0x1a, 0x4a, 0xc3, 0xc4, 0xde, 0x86, 0xfd, 0x7f, 0xe0, 0xc1,
                0x9c, 0x9f, 0x74, 0xc7, 0xc5, 0x2d, 0xcc, 0xe6, 0x9b, 0xa6, 0x5b, 0xa8, 0x62, 0x90, 0x59, 0x2b,
                0xe7, 0x5d, 0x64, 0xe6, 0x7a, 0x3d, 0xf3, 0x35, 0x53, 0x1b, 0x1a, 0xee, 0xa9, 0x16, 0x80, 0x47,
                0xbd, 0x7d, 0x80, 0x1f, 0xb9, 0xa7, 0x00, 0xf8, 0x33, 0x3e, 0x87, 0xf9, 0x6a, 0xcf, 0x3e, 0x11,
                0x70, 0x06, 0x7e, 0xcb, 0x49, 0xcb, 0x8d, 0x56, 0x9b, 0xbc, 0x7a, 0x33, 0xbc, 0x80, 0x3d, 0xcd,
                0x87, 0xda, 0xd7, 0x91, 0xc1, 0xbc, 0x62, 0x65, 0x0a, 0x1b, 0x05, 0x45, 0x09, 0x67, 0x56, 0x84,
                0xd2, 0x2c, 0x09, 0x0d, 0x94, 0xa1, 0xa3, 0x82, 0x71, 0x54, 0x66, 0xaa, 0x7f, 0x0c, 0xaa, 0xdb,
                0x19, 0x83, 0xe7, 0xa4, 0x72, 0x40, 0xe0, 0x19, 0x2a, 0xd5, 0xdc, 0x35, 0x3f, 0x3d, 0x64, 0x51,
                0x35, 0xa3, 0x41, 0xe5, 0x4e, 0x5b, 0x47, 0x6a, 0xa9, 0x83, 0x8a, 0x01, 0xf6, 0xbb, 0x74, 0x62,
                0x99, 0x64, 0x5b, 0x9d, 0x06, 0xc1, 0x3c, 0x5e, 0x5f, 0x33, 0x87, 0xb1, 0xc1, 0x3c, 0xd3, 0x44,
                0xa3, 0x60, 0x4e, 0xcf, 0x30, 0x89, 0x27, 0x1f, 0x7e, 0xd3, 0xf0, 0x08, 0x92, 0x0e, 0x72, 0x94,
                0x35, 0xcb, 0x94, 0xbe, 0x1e, 0x1c, 0x8e, 0xa5, 0x2f, 0xa4, 0x05, 0x9e, 0xa6, 0x8b, 0x99, 0x40,
                0x11, 0x0d, 0x13, 0xc0, 0x12, 0xd4, 0xd9, 0xde, 0x20, 0x76, 0x64, 0x03, 0x8d, 0xb0, 0x68, 0x68,
                0xd2, 0x68, 0x1b, 0x21, 0x65, 0x15, 0xed, 0xd9, 0x64, 0x46, 0xc7, 0x79, 0x32, 0x6f, 0xa0, 0xc3,
                0xc6, 0x98, 0x8d, 0x7c, 0x9e, 0xc8, 0xef, 0xfd, 0xcb, 0x41, 0xd7, 0x30, 0xbb, 0xc4, 0xf5, 0xfd,
                0xb1, 0xe5, 0x5e, 0x2a, 0xa7, 0x32, 0xe8, 0xfb, 0x9d, 0xca, 0x22, 0x53, 0x90, 0xe2, 0x08, 0x39,
                0x12, 0x76, 0xdc, 0xe0, 0x4c, 0xcd, 0xa5, 0x27, 0x99, 0x2a, 0x22, 0xae, 0xb6, 0x62, 0x80, 0x73,
                0x41, 0x71, 0x6f, 0x8a, 0x16, 0x1f, 0x9f, 0xde, 0xc2, 0x8f, 0x36, 0x5b, 0x4e, 0xc1, 0x51, 0x27,
                0xeb, 0x25, 0x1d, 0x8b, 0x69, 0x98, 0x96, 0xf0, 0x9c, 0x8d, 0x94, 0x96, 0xae, 0xec, 0xe9, 0x73,
                0x6f, 0x8f, 0xdc, 0x2e, 0x1e, 0xeb, 0x76, 0xc0, 0x1a, 0x9f, 0x66, 0x02, 0x30, 0x9d, 0xcf, 0xc0,
                0x0d, 0xc4, 0x46, 0x6a, 0x1d, 0x3e, 0xbb, 0x2a, 0x6c, 0x43, 0xb4, 0x25, 0x35, 0xcb, 0x1d, 0xd3,
                0x11, 0x29, 0xb6, 0x2e, 0x4d, 0xa5, 0x16, 0xb8, 0x58, 0x98, 0x96, 0x28, 0x02, 0x1b, 0xe7, 0xd3,
                0x66, 0x4a, 0x10, 0xa8, 0x1f, 0xf4, 0xf3, 0xb7, 0xa6, 0x53, 0x55, 0xb2, 0x65, 0x8c, 0x28, 0x38,
                0x3a, 0x14, 0x76, 0x52, 0xd9, 0xa2, 0x23, 0x9f, 0x79, 0xa0, 0x75, 0x05, 0x88, 0xa1, 0xca, 0xb8,
                0x2d, 0x90, 0xa9, 0x73, 0x17, 0xf6, 0x2e, 0x89, 0x37, 0x91, 0x33, 0x36, 0x9c, 0xcc, 0x6d, 0x49,
                0xa1, 0xd9, 0xb8, 0xe8, 0x1c, 0x3f, 0x6e, 0x47, 0xcc, 0xc1, 0x7d, 0x7a, 0x80, 0x48, 0xdc, 0x52,
                0xdb, 0xbf, 0xf0, 0xc7, 0xeb, 0x41, 0x64, 0xe4, 0xaf, 0xe9, 0x04, 0x9f, 0x73, 0x4f, 0xf6, 0x20
            };
            PSK right
            {
                0xf7, 0xae, 0x2b, 0xd2, 0x1c, 0x17, 0xf9, 0x44, 0x6c, 0xd4, 0x53, 0x11, 0x10, 0x6d, 0x3e, 0x9e,
                0x9d, 0xcd, 0xd2, 0x8b, 0x7a, 0x44, 0xae, 0x45, 0xc6, 0x51, 0xc7, 0x27, 0xfa, 0x7d, 0xc1, 0x43,
                0x47, 0xd5, 0x47, 0x96, 0xc8, 0x7e, 0x78, 0xd1, 0x3b, 0xde, 0xcd, 0x16, 0xee, 0xce, 0x0b, 0xba,
                0xe3, 0x15, 0xc8, 0x5a, 0xb6, 0x7a, 0xfe, 0x43, 0xce, 0xc5, 0x77, 0xa5, 0x86, 0xa6, 0x63, 0xd2,
                0xcf, 0xa3, 0xb8, 0x4b, 0xec, 0x49, 0x3b, 0xb2, 0x8f, 0x34, 0xdc, 0x3f, 0x06, 0x3a, 0xc1, 0xa8,
                0xc8, 0xa6, 0x4e, 0x98, 0xb6, 0x23, 0x2b, 0x95, 0xa9, 0x7f, 0x88, 0x9d, 0x93, 0xd2, 0x91, 0x13,
                0x20, 0x13, 0xa8, 0x80, 0x68, 0x44, 0x19, 0x56, 0x5c, 0x42, 0x9e, 0x43, 0xc3, 0xe8, 0xfc, 0x05,
                0xfa, 0x51, 0x37, 0x35, 0x30, 0x2a, 0x45, 0xed, 0x70, 0xea, 0x2f, 0xea, 0xd1, 0x1d, 0x2f, 0x0d,
                0x21, 0x4e, 0xea, 0x82, 0xe6, 0x05, 0x5a, 0x43, 0x32, 0x25, 0x76, 0x8e, 0xd0, 0xbd, 0xf0, 0xa3,
                0x13, 0xd0, 0x8a, 0x4e, 0xed, 0xee, 0x69, 0x46, 0x0b, 0xff, 0x7c, 0xe0, 0xd5, 0x48, 0xdd, 0x1f,
                0x3b, 0xc7, 0xb1, 0x02, 0xa1, 0x90, 0xaa, 0x6b, 0xee, 0x1d, 0xc6, 0x98, 0x03, 0xd3, 0x5c, 0x1d,
                0x25, 0xe8, 0x0c, 0xf4, 0x39, 0x57, 0xd4, 0xc2, 0x3e, 0xfc, 0x7e, 0x07, 0xa9, 0xf0, 0x4d, 0xba,
                0x60, 0xd4, 0x64, 0x86, 0x7c, 0x72, 0xd0, 0x6b, 0x87, 0x26, 0xdf, 0xcb, 0x9b, 0xa1, 0x9d, 0x67,
                0xa5, 0xcb, 0x99, 0x63, 0xd1, 0x72, 0x71, 0xb0, 0xe4, 0x6c, 0xd5, 0x83, 0xae, 0x9e, 0xc0, 0xa6,
                0x28, 0xcc, 0x87, 0xee, 0x65, 0xa6, 0x3f, 0x50, 0xaf, 0xb4, 0x61, 0x3b, 0xc0, 0x66, 0xad, 0x6d,
                0xb5, 0xa9, 0x56, 0x79, 0xcf, 0x8a, 0x85, 0x75, 0x88, 0xea, 0xa4, 0x0d, 0xe8, 0xb5, 0xe4, 0x75,
                0x0f, 0x35, 0xdf, 0x85, 0x1b, 0xbe, 0x15, 0xe2, 0xcd, 0xd9, 0xb8, 0x07, 0xf3, 0xb6, 0x18, 0xe0,
                0x54, 0x9c, 0xa4, 0x54, 0x0e, 0x00, 0x02, 0x1e, 0x9b, 0x8e, 0xab, 0x78, 0x2c, 0x3d, 0x07, 0x28,
                0xe0, 0xd0, 0xf8, 0x1f, 0xb9, 0x05, 0x1c, 0x0d, 0x81, 0x5d, 0x5d, 0x2c, 0x1c, 0x7c, 0xa4, 0x25,
                0xde, 0x6a, 0x51, 0xe3, 0x0d, 0x4e, 0xd7, 0x73, 0x24, 0xf5, 0x41, 0xe4, 0x8d, 0x19, 0x78, 0xe6,
                0xb6, 0xeb, 0xfd, 0x5a, 0x6e, 0x9d, 0x2e, 0x20, 0xe7, 0x67, 0xfa, 0x9d, 0x16, 0xfe, 0xd9, 0xc3,
                0x82, 0x70, 0x23, 0xdc, 0xe7, 0xde, 0x5a, 0x0a, 0xb5, 0xaf, 0xca, 0x86, 0xbf, 0x21, 0xea, 0xb8,
                0xa7, 0xae, 0xdc, 0x86, 0x41, 0xae, 0xb4, 0x6b, 0x9d, 0xc8, 0x86, 0x09, 0x53, 0x9e, 0x7a, 0xf1,
                0x66, 0x4f, 0xc9, 0xe6, 0x33, 0xd4, 0x5a, 0x10, 0x8e, 0x42, 0x60, 0x2b, 0xb0, 0xed, 0x57, 0xf3,
                0xc2, 0x9b, 0xba, 0xf3, 0x25, 0x00, 0x7f, 0x52, 0xb8, 0x89, 0x1e, 0x7e, 0xe1, 0xc1, 0x69, 0xd7,
                0x67, 0xba, 0xcc, 0x54, 0x36, 0xc5, 0x4e, 0xf4, 0x5a, 0x62, 0x1d, 0xe3, 0x5f, 0x50, 0xcf, 0xbf,
                0xcd, 0x7b, 0xe6, 0x7c, 0xd3, 0x8b, 0x50, 0xf6, 0xf0, 0xfe, 0xac, 0xfd, 0xdd, 0x42, 0x05, 0xd1,
                0x90, 0x7c, 0xe4, 0x73, 0x3e, 0x91, 0x7a, 0x66, 0x24, 0xd6, 0xdb, 0xb7, 0xf3, 0xa6, 0xb1, 0x9c,
                0x5f, 0x1f, 0x90, 0x48, 0x2e, 0x2a, 0x65, 0x7b, 0x76, 0x83, 0xd3, 0x0f, 0x08, 0x63, 0xad, 0x2d,
                0xd9, 0x61, 0xe2, 0x97, 0xb6, 0x80, 0xc2, 0xe2, 0x61, 0x1d, 0x3b, 0x68, 0xc6, 0xce, 0x21, 0x4b,
                0x80, 0x9d, 0x75, 0x82, 0xfb, 0x6c, 0xbe, 0x5d, 0xcf, 0xe1, 0xfe, 0xfc, 0x55, 0x78, 0x7c, 0x6b,
                0xa9, 0xe6, 0x45, 0xc9, 0x94, 0x6b, 0xce, 0xd3, 0x68, 0x35, 0xca, 0x65, 0xf3, 0x87, 0xff, 0x04,
                0xec, 0x24, 0xb3, 0xb2, 0xda, 0x83, 0xab, 0xc8, 0xeb, 0xf5, 0x52, 0x2d, 0xdc, 0xb1, 0x7c, 0xa1,
                0x7c, 0x08, 0x0b, 0xd0, 0xdd, 0x80, 0x8e, 0x19, 0x2c, 0x2d, 0x10, 0x76, 0x85, 0xed, 0x14, 0x14,
                0xc9, 0x0f, 0x0b, 0x74, 0x1e, 0x2a, 0x96, 0xc8, 0x1d, 0xa5, 0x42, 0x76, 0x5e, 0x1e, 0x0e, 0x40,
                0xa6, 0x1e, 0xe2, 0xc9, 0x29, 0x90, 0x28, 0x3c, 0xe8, 0xbc, 0x7c, 0x1d, 0x56, 0x8e, 0xe6, 0xb3,
                0x4d, 0x43, 0x1c, 0xa5, 0x0b, 0x34, 0x31, 0xcd, 0xee, 0x85, 0x4f, 0x98, 0x10, 0x24, 0x79, 0x43,
                0x3d, 0x49, 0x8d, 0xb1, 0x93, 0xa9, 0xaa, 0x5a, 0x05, 0x42, 0xf4, 0xf8, 0x3e, 0x9c, 0x71, 0xfe,
                0x4f, 0xc7, 0xe4, 0x08, 0xb5, 0xa3, 0xa5, 0xe1, 0x7f, 0x8a, 0xf4, 0x50, 0x72, 0xfa, 0x7f, 0xeb,
                0x06, 0xb9, 0x7c, 0x4c, 0x97, 0x0b, 0x24, 0x01, 0xb0, 0xc7, 0xf7, 0xa6, 0x2f, 0x38, 0x40, 0x49,
                0xe0, 0x39, 0x31, 0x86, 0xf2, 0x51, 0xc7, 0x17, 0xa4, 0x68, 0x30, 0xcb, 0xab, 0x23, 0x21, 0xc8,
                0x6c, 0x89, 0x94, 0x9e, 0x99, 0xca, 0x7b, 0x3a, 0xec, 0xea, 0x8a, 0x4b, 0xa3, 0x44, 0xbe, 0x7f,
                0x53, 0xa7, 0x75, 0xad, 0x7f, 0x58, 0xc1, 0xc6, 0x6c, 0xbb, 0xa5, 0x9e, 0x97, 0x7a, 0x59, 0x6c,
                0x35, 0x3a, 0xe3, 0x82, 0x74, 0x46, 0xe6, 0xfa, 0xf3, 0x13, 0xc0, 0x35, 0x48, 0x49, 0xa3, 0x57,
                0x49, 0x6f, 0x5a, 0x50, 0x5e, 0x86, 0x96, 0x8e, 0xf1, 0x04, 0x84, 0x78, 0x81, 0xf7, 0x32, 0x9c,
                0x64, 0x3d, 0x49, 0x72, 0xe6, 0xe7, 0xd4, 0xb5, 0x42, 0x79, 0x37, 0x72, 0xca, 0x35, 0xaf, 0xac,
                0xf1, 0x66, 0xee, 0x9f, 0xe4, 0x00, 0xcf, 0x22, 0x8b, 0xb2, 0xe5, 0x0c, 0xb1, 0x6b, 0x58, 0xdc,
                0xed, 0x93, 0x8e, 0x47, 0x8c, 0x22, 0x03, 0xa4, 0x31, 0x5c, 0xf4, 0x48, 0x1f, 0x56, 0x70, 0xd5,
                0x36, 0x7f, 0xb9, 0x1d, 0x73, 0x77, 0x62, 0x96, 0x04, 0x9a, 0x11, 0x8f, 0x2b, 0x45, 0x32, 0x22,
                0x65, 0xf7, 0xb0, 0x04, 0xa9, 0x5b, 0xc0, 0x84, 0x13, 0x5f, 0x59, 0x12, 0x52, 0xd8, 0x36, 0x2b,
                0xca, 0x8b, 0xc3, 0x51, 0xc2, 0x82, 0x79, 0x82, 0x82, 0x42, 0xa4, 0xa6, 0xbc, 0x1d, 0x12, 0xa1,
                0x02, 0x47, 0xee, 0x98, 0x11, 0x13, 0xa9, 0x2a, 0x63, 0x1e, 0xde, 0x2e, 0x8d, 0xff, 0x82, 0x73,
                0xe6, 0xaf, 0xec, 0xba, 0xb9, 0xa4, 0x64, 0x5d, 0xd1, 0xf1, 0x18, 0x41, 0x31, 0xf7, 0x5a, 0x07,
                0xfc, 0xd2, 0x45, 0xd1, 0xb7, 0x77, 0x05, 0x42, 0x3a, 0x7d, 0x0c, 0xde, 0x2a, 0xc2, 0x49, 0xe8,
                0x39, 0xfc, 0x8f, 0x18, 0x84, 0xbe, 0x88, 0xdd, 0x06, 0x76, 0xbb, 0x85, 0x6c, 0x32, 0x61, 0x51,
                0x6f, 0xae, 0x03, 0xac, 0x6f, 0x15, 0x49, 0x10, 0x3c, 0x9a, 0x6d, 0x8f, 0xff, 0x8b, 0x97, 0x55,
                0xc6, 0xbe, 0x6c, 0x0c, 0xd5, 0x22, 0xf2, 0x2a, 0x7a, 0x44, 0x50, 0xee, 0xfb, 0xa5, 0xf6, 0xc6,
                0x68, 0x12, 0x0f, 0x75, 0x9a, 0xcc, 0x96, 0xdb, 0x7e, 0x2a, 0x6b, 0x5c, 0xb1, 0xaf, 0x9e, 0xb9,
                0x26, 0xac, 0x5e, 0xd9, 0xcc, 0x5c, 0x5e, 0x42, 0x47, 0x73, 0x2a, 0x71, 0x9e, 0x1b, 0x8e, 0xb8,
                0xa0, 0xb7, 0x19, 0x0a, 0x1a, 0xe9, 0xe3, 0x13, 0xf7, 0xcf, 0xcb, 0xcc, 0x65, 0x80, 0xcf, 0xfd,
                0x1c, 0xde, 0x2c, 0x8c, 0xf6, 0xb9, 0x9a, 0x05, 0x6c, 0xf6, 0x90, 0xa7, 0x08, 0xa0, 0x75, 0x9f,
                0xa2, 0xb9, 0xce, 0x3f, 0x34, 0xdb, 0x28, 0x57, 0x03, 0xe6, 0x6d, 0x4b, 0x62, 0xb9, 0x6c, 0x01,
                0x5d, 0xad, 0x97, 0x2e, 0x29, 0xb8, 0xc4, 0xfb, 0x2e, 0x95, 0xb4, 0x7d, 0xf4, 0xb1, 0x19, 0x0d,
                0x9d, 0x4c, 0x09, 0xcb, 0xfd, 0x8b, 0x68, 0x12, 0x7b, 0x26, 0x4c, 0xcd, 0x1e, 0x8a, 0xbf, 0xdf
            };

            PSK result = left;
            const size_t itterations = 1000;

            result ^= right;
            for(size_t index = 0; index < left.size(); index++)
            {
                ASSERT_EQ(result[index], left[index] ^ right[index]) << "Xor invalid value";
            }

            auto startTime = std::chrono::high_resolution_clock::now();

            for(size_t count = 0; count < itterations; count++)
            {
                result ^= right;
            }
            auto duration = std::chrono::high_resolution_clock::now() - startTime;

            startTime = std::chrono::high_resolution_clock::now();

            for(size_t count = 0; count < itterations; count++)
            {
                for(size_t index = 0; index < left.size(); index++)
                {
                    result[index]= left[index] ^ right[index];
                }
            }
            auto controlDuration = std::chrono::high_resolution_clock::now() - startTime;

            using namespace std::chrono;
            double speedup = duration_cast<nanoseconds>(controlDuration).count() / duration_cast<nanoseconds>(duration).count();
            RecordProperty("xorspeedup", speedup);
            std::cout << "Multi byte xor took:  " << duration_cast<nanoseconds>(duration).count() << "ns\n";
            std::cout << "Single byte xor took: " << duration_cast<nanoseconds>(controlDuration).count() << "ns\n";
            std::cout << "Speedup is: " << speedup << "x\n";
            ASSERT_LT(duration, controlDuration) << "Multibyte is no faster the byte by byte";
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
