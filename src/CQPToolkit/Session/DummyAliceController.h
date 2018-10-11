/*!
* @file
* @brief DummyController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 9/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Session/SessionController.h"
#include "CQPToolkit/Util/WorkerThread.h"
#include "QKDInterfaces/IDetector.grpc.pb.h"
#include "CQPToolkit/Session/Stats.h"

namespace cqp
{
    namespace sim
    {
        class DummyTransmitter;
    }
    namespace align
    {
        class TransmissionHandler;
    }
    namespace sift
    {
        class Transmitter;
    }
    namespace ec
    {
        class ErrorCorrection;
    }
    namespace privacy
    {
        class PrivacyAmplify;
    }
    namespace keygen
    {
        class KeyConverter;
    }

    namespace session
    {

        /**
         * @brief The DummyAliceController class
         * Provides a software only QKD device - Transmists photons
         */
        class CQPTOOLKIT_EXPORT DummyAliceController : public SessionController,
            protected WorkerThread
        {
        public:
            /**
             * @brief DummyAliceController
             * Constructor
             * @param creds credentials used to connect to peer
             * @param bytesPerKey
             */
            DummyAliceController(std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey = 16);

            /// Disctructor
            ~DummyAliceController() override;

            ///@{
            /// @name WorkerThread

            /**
             * @brief DoWork
             *
             */
            void DoWork() override;

            /// @}

            ///@{
            /// @name ISessionController interface

            /// @copydoc ISessionController::StartSession
            grpc::Status StartSession(const remote::OpticalParameters& params) override;
            /// @copydoc ISessionController::EndSession
            void EndSession() override;

            /// @copydoc cqp::ISessionController::GetKeyPublisher
            IKeyPublisher* GetKeyPublisher() override;

            /// @copydoc cqp::ISessionController::GetStats
            virtual std::vector<stats::StatCollection*> GetStats() const override;
            ///@}

            ///@{
            /// @name Session interface

            /// @copydoc cqp::remote::ISession::SessionStarting
            /// @param context Connection details from the server
            grpc::Status SessionStarting(grpc::ServerContext* context, const remote::SessionDetails* request, google::protobuf::Empty*) override;

            /// @copydoc cqp::remote::ISession::SessionEnding
            /// @param context Connection details from the server
            grpc::Status SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty*, google::protobuf::Empty*) override;

            ///@}

        protected: // methods
            ///@{
            /// @name SessionController overrides

            /// @copydoc SessionController::RegisterServices
            void RegisterServices(grpc::ServerBuilder& builder) override;
            ///@}

            /**
             * @brief ConnectRemoteChain
             * connect each stage to it's remote partner
             */
            void ConnectRemoteChain();

            /// Statistics produced by this class
            Statistics stats;
        protected: // members
            /// Produces photons
            std::shared_ptr<sim::DummyTransmitter> photonGen = nullptr;
            /// remote connection to the detector
            std::unique_ptr<remote::IDetector::Stub> detector;
            /// aligns detections
            std::shared_ptr<align::TransmissionHandler> alignment = nullptr;
            /// sifts alignments
            std::shared_ptr<sift::Transmitter> sifter = nullptr;
            /// error corrects sifted data
            std::shared_ptr<ec::ErrorCorrection> ec = nullptr;
            /// verify corrected data
            std::shared_ptr<privacy::PrivacyAmplify> privacy = nullptr;
            /// prepare keys for the keystore
            std::shared_ptr<keygen::KeyConverter> keyConverter = nullptr;
        };

    } // namespace session
} // namespace cqp


