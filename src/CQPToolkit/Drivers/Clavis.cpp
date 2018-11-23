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
#include "Clavis.h"
#include "CQPToolkit/Util/Logger.h"
#include <iomanip>
#include "CQPToolkit/Util/Util.h"
#include "CQPToolkit/Net/Socket.h"
#include "CQPToolkit/Datatypes/Keys.h"
#include <thread>

namespace cqp
{

    Clavis::Clavis(const std::string& address, bool isAlice, uint8_t inDeviceId, uint8_t keyLength) :
        deviceId(inDeviceId),
        hardwareAddress(address),
        alice(isAlice)
    {
        using namespace std;

        // wait for one second for response
        socket.SetReceiveTimeout(std::chrono::milliseconds(5000));

        if(hardwareAddress.GetHost().empty())
        {
            hardwareAddress.SetHost("localhost");
        }

        if(hardwareAddress.GetPort() == 0)
        {
            hardwareAddress.SetPort(DefaultPort);
        }

        myKeyLength = keyLength;
        if(deviceId < 1 || deviceId > Clavis::MAX_DEV_ID)
        {
            LOGWARN("Invalid device ID, resetting to 1");
            deviceId = 1;
        }
    }

    bool Clavis::BeginKeyTransfer(const ClavisKeyID& keyID)
    {
        using namespace std;

        bool result = true;

        // populate the header to send to the device
        KeyRequest newRequest = {};
        newRequest.header.datagramType = DatagramType::KeyRequest;
        newRequest.header.SetKeyId(keyID);
        newRequest.header.keyLength = myKeyLength;
        newRequest.header.keyRequestID = ++currentRequestId;
        newRequest.requestingDeviceID = deviceId;

        // Calculate the CRC of the message
        newRequest.crc32 = CRCFddi(&newRequest, sizeof(newRequest) - sizeof(newRequest.crc32));

        LOGDEBUG("Sending key request: KeyID=" + to_string(keyID) + " length=" + to_string(myKeyLength));

        //newRequest.ToNetworkOrder();
        try
        {
            // Send the request to the device
            socket.SendTo(
                &newRequest, sizeof(newRequest),
                hardwareAddress
            );
        }
        catch (const exception& e)
        {
            result = false;
            LOGERROR(e.what());
        }

        return result;
    }

    bool Clavis::ReadKeyResponse(PSK& newKey, ClavisKeyID& keyId, ErrorStatus& status)
    {
        using namespace std;

        bool result = true;
        KeyResponse response = {};
        net::SocketAddress expectedSender;
        net::SocketAddress sender;

        if(!hardwareAddress.ResolveAddress(expectedSender))
        {
            LOGERROR("Failed to resolve: " + hardwareAddress.ToString());
        }

        // Try to get a packet from the device
        size_t bytesRead = 0;
        result = socket.ReceiveFrom(
                     &response, sizeof(response), bytesRead, sender
                 );

        if(result)
        {
            if (sender != expectedSender)
            {
                LOGWARN("Unknown sender:" + sender.ToString() + "!=" + expectedSender.ToString());
            }

            // validate the message
            if (bytesRead != sizeof(response) || response.header.datagramType != DatagramType::KeyResponse ||
                    response.header.keyLength > Clavis::MaxKeyLength)
            {
                result = false;
                LOGERROR("Invalid packet from " + sender.ToString());
            }

            LOGDEBUG("Message received");
        }
        else
        {
            // request timed out
            status = ErrorStatus::NoMoreKeys;
        }

        if(result)
        {
            //response.ToHostOrder();
            // check the message crc
            uint32_t inCRC = CRCFddi(&response, sizeof(response) - sizeof(response.crc32));
            if(inCRC != response.crc32)
            {
                result = false;
                status = ErrorStatus::InvalidKeyRequest;
                LOGERROR("CRC Missmatch. Got " + to_string(response.crc32) + " Expected " + to_string(inCRC));
            }
            else if(response.errorStatus != ErrorStatus::Success)
            {
                if(response.errorStatus != ErrorStatus::NoMoreKeys)
                {
                    LOGWARN("Key response error: " + string(ErrorStatusString[static_cast<int>(response.errorStatus)]));
                }
                result = false;
                status = response.errorStatus;
            }
            else
            {
                status = response.errorStatus;
            }
        }

        if(result && response.errorStatus == ErrorStatus::Success)
        {
            LOGDEBUG("KeyResponse ID=" + to_string(response.header.GetKeyId()) +
                     " ReqID=" + to_string(response.header.keyRequestID) +
                     " DevID=" + to_string(response.requestingDeviceID) +
                     " Length=" + to_string(response.header.keyLength));

            // The message is good, copy out the data and it's id so it can be sent to the other side
            newKey.assign(response.key, response.key + response.header.keyLength);

            if(myKeyLength != newKey.size())
            {
                LOGERROR("Provided key length != requested key length");
            }
            // we only do one key at a time
            keyId = response.header.GetKeyId();
            status = response.errorStatus;
        }

        return result;
    }

    bool Clavis::GetExistingKey(PSK& newKey, const ClavisKeyID& keyId)
    {
        ClavisKeyID myKeyId = keyId;
        return GetKey(newKey, myKeyId);
    }



    bool Clavis::GetNewKey(PSK& newKey, ClavisKeyID& keyId)
    {
        keyId = 0;
        return GetKey(newKey, keyId);
    }



    bool Clavis::GetKey(PSK& newKey, ClavisKeyID& keyId)
    {
        using namespace std;
        bool result = false;
        abortRequest = false;
        ErrorStatus status = ErrorStatus::InvalidKeyRequest;

        int numRequests = requestRetryLimit;

        do
        {
            // requesting 0 will generate a new key id
            if(BeginKeyTransfer(keyId))
            {
                if(ReadKeyResponse(newKey, keyId, status))
                {
                    LOGDEBUG("Successful key generation.");
                    result = true;
                }
                else
                {
                    switch(status)
                    {
                    case ErrorStatus::Success:
                        LOGERROR("Strange response from ReadKeyResponse");
                        break;
                    case ErrorStatus::NoMoreKeys:

                        LOGDEBUG("No more key, waiting...");
                        if(requestRetryLimit > 0)
                        {
                            numRequests--;
                        }

                        this_thread::sleep_for(chrono::seconds(10));
                        break;
                    case ErrorStatus::KeyIDDoesntExist:
                    case ErrorStatus::WrongKeyLength:
                    case ErrorStatus::InvalidKeyRequest:
                        LOGERROR("Key request failed");
                        break;
                    } // switch(status)
                } // if(ReadKeyResponse(newKey, keyId, status))
            }
            else
            {
                LOGERROR("Failed to begin key transfer.");
            } // if(BeginKeyTransfer(keyId))

            // repeat if there are no more keys
        }
        while(!abortRequest && status == ErrorStatus::NoMoreKeys && numRequests != 0);

        return result;
    }

}
