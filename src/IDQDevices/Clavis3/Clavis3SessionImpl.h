/*!
* @file
* @brief %{Cpp:License:ClassName}
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 15/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#if defined(HAVE_IDQ4P)
#include "Clavis3Device.h"
#include <zmq.hpp>
#include "SystemState.hpp"
#include "SignalId.hpp"
#include <atomic>
#include <thread>
#include "QKDInterfaces/Site.pb.h"
#include "IDQDevices/idqdevices_export.h"

#include "CQPToolkit/Alignment/Stats.h"
#include "CQPToolkit/ErrorCorrection/Stats.h"

namespace idq4p
{
    namespace classes
    {
        class GetBoardInformation;
        class GetProtocolVersion;
        class GetSoftwareVersion;
    }
}

namespace cqp
{

    /**
     * @brief The Clavis3Device::Impl class manages the conneciton with the Clavis 3 device
     */
    class IDQDEVICES_NO_EXPORT Clavis3Session::Impl
    {
    public:

        /**
         * @brief Impl constructor
         * @param address The hostname (no port) of the device to connect to
         */
        Impl(const std::string& address);

        /// distructor
        ~Impl();

        /**
         * @brief Request_UpdateSoftware
         * @param filename The name of the file previously uploaded to the devices `/tmp` folder
         * @param filenameSha1 The sha1 hash of the file uploaded
         */
        void Request_UpdateSoftware(const std::string& filename, const std::string& filenameSha1);

        /**
         * @brief Zeroize
         * Clear all QKD internal buffers. Clear also the authentication key.
         */
        void Zeroize();

        /**
         * @brief SetInitialKey
         * Set the initial key for authentication.
         * This command should be used at each boot of the QKE, when it is in the state ExecutingSecurityInitialization.
         * The size of the initial key shall be of 25 kbits (3125 Bytes).
         * @param key
         */
        void SetInitialKey(const DataBlock& key);

        /**
         * @brief GetRandomNumber
         * Get a random number from Quantis contained into QKD. numberSize: size of the random number in bytes.
         * Maximal allowed size = 4096 [bytes]. The operational rate of this command is 100[kbps]. The peak rate is
         *  1[Mbps] during 1 second.
         * @param out The random number
         * @return true on success
         */
        bool GetRandomNumber(std::vector<uint8_t>& out);

        /**
         * @brief GetProtocolVersion
         * Get the version of the IDQ4P communication protocol.
         * major: major number
         * minor: minor number
         * revision: revision number
         *
         * The major number is increased when there are significant jumps in functionality such as changing the framework
         * which could cause incompatibility with interfacing systems, the minor number is incremented when only minor
         * features or significant fixes have been added, and the revision number is incremented when minor bugs are fixed.
         * @return The response from the device
         */
        idq4p::classes::GetProtocolVersion GetProtocolVersion();

        /**
         * @brief The SoftwareId enum for specifying which software versio to get with GetSoftwareVersion
         */
        enum class SoftwareId : int32_t
        {
            CommunicatorService = 1,
            BoardSupervisorService = 2,
            RegulatorServiceAlice = 3,
            RegulatorServiceBob = 4,
            FpgaConfiguration = 5
        };

        /**
         * @brief GetSoftwareVersion
         * Taken from protocol defintion pdf version 0.11.0
         * Get the version of the software specified by softwareId. SoftwareId may be one of:
         * CommunicatorService = 1
         * BoardSupervisorService = 2
         * RegulatorServiceAlice = 3
         * RegulatorServiceBob = 4
         * FpgaConfiguration = 5
         * @return
         */
        idq4p::classes::GetSoftwareVersion GetSoftwareVersion(SoftwareId whichSoftware);

        /**
         * @brief The BoardId enum for requesting the board details with GetBoardInformation
         * Taken from protocol defintion pdf version 0.11.0
         */
        enum class BoardId : int32_t
        {
            QkeComE = 1,
            QkeHost = 2,
            QkeAlice = 3,
            QkeBob = 4,
            QkeFpga = 5
        };

        /**
         * @brief GetBoardInformation
         * Get the information on the board specified by the boardId. BoardId may be one of:
         * QkeComE = 1
         * QkeHost = 2
         * QkeAlice = 3
         * QkeBob = 4
         * QkeFpga = 5
         * @return
         */
        idq4p::classes::GetBoardInformation GetBoardInformation(BoardId whichBoard);

        /**
         * @brief PowerOn
         * Command used for automatic startup of the QKD.
         * This command will cause the emission of the following signals in this sequence, which inform the subscriber
         *  about the state of the startup process
         */
        void PowerOn();

        /**
         * @brief PowerOff
         * Shut down the system in a clean way. 4, 5, 7 This command will cause the emission of the following signals in
         * this sequence, which inform the subscriber about the state of the shutdown process
         */
        void PowerOff();

        /**
         * @brief SubscribeToSignals
         * Subscribe to the handlable signals
         */
        void SubscribeToSignals();

        /**
         * @brief ReadKey
         * Request a key from the device
         * @param[out] keyValue
         * @return true on success
         */
        bool ReadKey(PSK& keyValue);

        /**
         * @brief GetSide
         * @return the side of the connected device
         */
        remote::Side::Type GetSide() const;

        cqp::align::Statistics alignementStats;
        cqp::ec::Stats errorStats;

    protected: // methods

        /**
         * @brief ReadSignalSocket
         * Process incomming from the device
         */
        void ReadSignalSocket();

        /**
         * @brief SubscribeToSignal
         * Request that the device sends signals
         * @param sig signal to recieve
         */
        void SubscribeToSignal(idq4p::domainModel::SignalId sig);

        /**
         * @brief UnsubscribeSignal
         * @param sig signal to stop recieving
         */
        void UnsubscribeSignal(idq4p::domainModel::SignalId sig);
    protected: // members

        static constexpr const uint16_t managementPort = 5561;
        static constexpr const uint16_t keyChannelPort = 5560;
        static constexpr const uint16_t signalsPort = 5562;

        zmq::context_t context {1};
        zmq::socket_t mgmtSocket{context, ZMQ_REQ};
        zmq::socket_t signalSocket{context, ZMQ_SUB};
        zmq::socket_t keySocket{context, ZMQ_SUB};

        std::atomic_bool shutdown {false};
        std::atomic<idq4p::domainModel::SystemState> state;
        std::thread signalReader;
        remote::Side::Type side;
    };

}
#endif // HAVE_IDQ4P
