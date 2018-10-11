/*!
* @file
* @brief DummyBobController
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

namespace cqp
{

    namespace sim
    {
        class DummyTimeTagger;
    }
    namespace align
    {
        class DetectionReciever;
    }
    namespace sift
    {
        class Receiver;
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
         * @brief The DummyBobController class
         * Provides a software only QKD device - Detects photons
         */
        class CQPTOOLKIT_EXPORT DummyBobController : public SessionController
        {
        public:
            /**
             * @brief DummyBobController
             * @param creds credentials used to connect to peer
             * @param bytesPerKey
             */
            DummyBobController(std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey = 16);

            ~DummyBobController() override;

            ///@{
            /// @name ISessionController interface

            /// @copydoc ISessionController::StartSession
            grpc::Status StartSession(const remote::OpticalParameters& params) override;
            /// @copydoc ISessionController::EndSession
            void EndSession() override;

            /// @copydoc cqp::ISessionController::GetKeyPublisher
            Provider<IKeyCallback>* GetKeyPublisher() override;

            ///@}

        protected: // methods
            ///@{
            /// @name SessionController overrides

            /// @copydoc SessionController::RegisterServices
            virtual void RegisterServices(grpc::ServerBuilder& builder) override;

            /// @copydoc cqp::ISessionController::GetStats
            virtual std::vector<stats::StatCollection*> GetStats() const override;

            /// @copydoc cqp::remote::ISession::SessionStarting
            /// @param context Connection details from the server
            grpc::Status SessionStarting(grpc::ServerContext* context, const remote::SessionDetails* request, google::protobuf::Empty*response) override;

            /// @copydoc cqp::remote::ISession::SessionEnding
            /// @param context Connection details from the server
            grpc::Status SessionEnding(grpc::ServerContext* context, const google::protobuf::Empty*, google::protobuf::Empty*) override;

            ///@}

        protected: // members
            /// detects photons
            std::shared_ptr<sim::DummyTimeTagger> timeTagger = nullptr;
            /// aligns detections
            std::shared_ptr<align::DetectionReciever> alignment = nullptr;
            /// sifts alignments
            std::shared_ptr<sift::Receiver> sifter = nullptr;
            /// error corrects sifted data
            std::shared_ptr<ec::ErrorCorrection> ec = nullptr;
            /// verify corrected data
            std::shared_ptr<privacy::PrivacyAmplify> privacy = nullptr;
            /// prepare keys for the keystore
            std::shared_ptr<keygen::KeyConverter> keyConverter = nullptr;
        };
    } // namespace session
} // namespace cqp


