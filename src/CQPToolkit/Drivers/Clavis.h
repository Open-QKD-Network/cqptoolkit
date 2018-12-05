/*!
* @file
* @brief CQP Toolkit - IDQ Clavis Driver
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 16 May 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Net/Sockets/Datagram.h"
#include "Algorithms/Datatypes/URI.h"
#include <cstdint>
#include <string>
#include "CQPToolkit/cqptoolkit_export.h"

namespace cqp
{
    class PSK;

    /// Provides access to the Clavis devices from ID Quantique
    class CQPTOOLKIT_EXPORT Clavis
    {

    public:
        /// The standard port which IDQSequence runs on
        static constexpr const uint16_t DefaultPort = 5323;
        /// The highest device id the clavis will accept
        static constexpr const uint8_t MAX_DEV_ID = 12;
        /// The largest number of bits the clavis will return for a key
        static constexpr const uint8_t MaxKeyLength = 32;
        /// Identifier for the key requested/received
        using ClavisKeyID = uint64_t;

        Clavis() {}

        /// Construct a driver for one hardware device
        /// @param[in] address The address and port of the device.
        /// @param[in] isAlice If this device is alice
        /// @param[in] inDeviceId Identifier to distinguish this instance with other instances
        ///     which may be using the same hardware.
        /// @param[in] keyLength The number of bytes each request will contain.
        Clavis(const std::string& address, bool isAlice, uint8_t inDeviceId = 1, uint8_t keyLength = MaxKeyLength);

        /// Request and emit a particular key from the device
        /// @param[out] newKey The secret key bytes
        /// @param[in] keyId The id which has been requested by the other side
        /// @return true if the key was successfully obtained
        bool GetExistingKey(PSK& newKey, const Clavis::ClavisKeyID& keyId);

        /// Request and emit a new key
        /// @param[out] newKey The secret key bytes
        /// @param[out] keyId The identifier for the new key
        /// @return true if the key was successfully obtained
        bool GetNewKey(PSK& newKey, Clavis::ClavisKeyID& keyId);

        /// Change the number of times a key is requested when there are no key available before giving up
        /// The default is to retry forever
        /// @param[in] limit The number of retries. -1 = forever
        void SetRequestRetryLimit(int limit)
        {
            requestRetryLimit = limit;
        }

        /// Returns the identifier for this unit
        /// @returns unit id
        uint8_t GetDeviceId() const
        {
            return deviceId;
        }
        /// Returns the number of bytes within each key emitted by GetExistingKey or GetNewKey
        /// @returns bytes within each key
        uint8_t GetKeyLength() const
        {
            return myKeyLength;
        }

        /// Stop requesting key if the current request is retrying
        /// Has no effect if there is no currently active request.
        void AbortRequest()
        {
            abortRequest = true;
        }

        /**
         * @brief IsAlice
         * @return true if this device is alice
         */
        bool IsAlice() const
        {
            return alice;
        }

        /// Standard destructor
        virtual ~Clavis() = default;

    protected:

        /// Possible error codes resulting from requesting a key
        enum class ErrorStatus : uint8_t
        {
            Success = 0,
            NoMoreKeys,
            KeyIDDoesntExist,
            WrongKeyLength,
            InvalidKeyRequest
        };
        /// the identifier for this unit
        uint8_t deviceId = 0;
        /// Counter used for sending messages to the device
        uint8_t currentRequestId = 0;
        /// Number of bytes returned by the device when requesting a key.
        uint8_t myKeyLength = Clavis::MaxKeyLength;
        /// The devices udp address
        net::SocketAddress hardwareAddress;
        /// Socket on which requests are sent
        net::Datagram socket;
        /// Number of times to retry when there is no more key
        /// -1 = forever
        int requestRetryLimit = -1;
        /// Stop requesting key if the current request is retrying
        bool abortRequest = false;

        /// is this device alice
        bool alice = false;

        /// Handle the key response.
        /// @param[out] newKey The key data extracted from the packet if status is success
        /// @param[out] keyId The id of the new key
        /// @param[out] status The error status sent with the message
        /// @return True if the packet was decoded correctly
        bool ReadKeyResponse(PSK& newKey, ClavisKeyID& keyId, ErrorStatus& status);

        /// Request a key from the previously opened device.
        /// @param[in] keyID The specific key to request
        /// @details Multiple requests for the same keyid will return the same key data
        ///     For unique key on every request a new keyid must be requested.
        /// @return true if the request was sent successfully
        bool BeginKeyTransfer(const ClavisKeyID& keyID);

        /// Request and emit a key
        /// If keyId = 0, a new key will be requested and keyId will be filled with the new id
        /// @param[out] newKey The secret key bytes
        /// @param[in,out] keyId The identifier for the key
        /// @return true if the key was successfully obtained
        bool GetKey(PSK& newKey, ClavisKeyID& keyId);

#pragma pack(push, 1)
        /// Type of the datagram
        enum class DatagramType : uint8_t
        {
            KeyRequest = 0,
            KeyResponse
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

            /// Store the new key ID
            /// @param newId Change the stored key id
            void SetKeyId(ClavisKeyID newId)
            {
                // Shift right by the number of bits in the lower word
                keyIDhw = newId >> (sizeof(keyIDlw) * 8);
                // force the lower bytes into the lower word
                keyIDlw = static_cast<uint32_t>(newId);
            }

            /// Returns the current value of the key ID.
            /// @return the current key id
            ClavisKeyID GetKeyId()
            {
                ClavisKeyID result = static_cast<ClavisKeyID>(keyIDlw);
                // Shift left by the number of bits in the lower word.
                result += static_cast<ClavisKeyID>(keyIDhw) << (sizeof(keyIDlw) * 8);
                return result;
            }

        };

        /// Datagram for requesting a key from the device
        struct KeyRequest
        {
            /// @copydoc KeyHeader
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

        /// Datagram response from requesting a key from the device
        struct KeyResponse
        {
            /// @copydoc KeyHeader
            KeyHeader           header;
            /// Indicates success or failure of the Key Response.
            ErrorStatus         errorStatus;
            /// The requested or new key. Only the number of
            /// octets specified in the KeyLength parameter is used
            /// as a key.
            uint8_t             key[Clavis::MaxKeyLength];
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

    }; //class Clavis
} //namespace cqp
