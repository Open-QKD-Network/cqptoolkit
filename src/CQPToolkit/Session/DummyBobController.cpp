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
#include "DummyBobController.h"
#include "CQPToolkit/Simulation/DummyTimeTagger.h"
#include "CQPToolkit/Alignment/DetectionReciever.h"
#include "CQPToolkit/Sift/Receiver.h"
#include "CQPToolkit/ErrorCorrection/ErrorCorrection.h"
#include "CQPToolkit/PrivacyAmp/PrivacyAmplify.h"
#include "CQPToolkit/KeyGen/KeyConverter.h"
#include "CQPToolkit/Net/TwoWayServerConnector.h"

namespace cqp
{
    namespace session
    {
        using google::protobuf::Empty;
        using grpc::Status;
        using grpc::ClientContext;

        DummyBobController::DummyBobController(std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey) :
            SessionController(creds)
        {
            timeTagger.reset(new sim::DummyTimeTagger(rng));
            // TODO: Fix these values
            alignment.reset(new align::DetectionReciever());
            sifter.reset(new sift::Receiver());
            ec.reset(new ec::ErrorCorrection());
            privacy.reset(new privacy::PrivacyAmplify());
            keyConverter.reset(new keygen::KeyConverter(bytesPerKey));

            // attach each stage to the next in the chain
            timeTagger->Attach(alignment.get());
            alignment->Attach(sifter.get());
            sifter->Attach(ec.get());
            ec->Attach(privacy.get());
            privacy->Attach(keyConverter.get());
        } // DummyBobController

        DummyBobController::~DummyBobController()
        {
            // disconnect the chain
            timeTagger->Detatch();
            alignment->Detatch();
            sifter->Detatch();
            ec->Detatch();
        } // ~DummyBobController

        grpc::Status DummyBobController::StartSession(const remote::OpticalParameters &params)
        {
            // stession is being started locally
            Status result = SessionController::StartSession(params);
            if(result.ok())
            {
                // connect each stage to it's remote partner
                alignment->Connect(twoWayComm->GetClient());

            }
            return result;
        }

        void DummyBobController::EndSession()
        {
            alignment->Disconnect();
        }

        Provider<IKeyCallback>* DummyBobController::GetKeyPublisher()
        {
            return keyConverter.get();
        } // GetKeyPublisher

        void DummyBobController::RegisterServices(grpc::ServerBuilder& builder)
        {
            builder.RegisterService(static_cast<remote::IPhotonSim::Service*>(timeTagger.get()));
            builder.RegisterService(static_cast<remote::IDetector::Service*>(timeTagger.get()));
            builder.RegisterService(sifter.get());
            builder.RegisterService(ec.get());
            builder.RegisterService(privacy.get());
            // ^^^ Add new services here ^^^ //
        } // RegisterServices

        std::vector<stats::StatCollection*> DummyBobController::GetStats() const
        {
            return
            {
                &timeTagger->stats,
                &alignment->stats,
                &sifter->stats,
                &ec->stats,
                &privacy->stats
            };
        }

        grpc::Status DummyBobController::SessionStarting(grpc::ServerContext *context, const remote::SessionDetails *request, google::protobuf::Empty *response)
        {
            // session is being started remotely
            Status result = SessionController::SessionStarting(context, request, response);
            if(result.ok())
            {
                // connect each stage to it's remote partner
                alignment->Connect(twoWayComm->GetClient());
            }
            return result;
        }

        grpc::Status DummyBobController::SessionEnding(grpc::ServerContext *context, const google::protobuf::Empty *request, google::protobuf::Empty *response)
        {
            Status result = SessionController::SessionEnding(context, request, response);
            if(result.ok())
            {
                alignment->Disconnect();
            }
            return result;
        }
    } // namespace session
} // namespace cqp
