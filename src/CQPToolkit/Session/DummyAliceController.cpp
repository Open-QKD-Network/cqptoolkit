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
#include "DummyAliceController.h"
#include "Util/Logger.h"
#include "CQPToolkit/Simulation/DummyTransmitter.h"
#include "CQPToolkit/Interfaces/IPhotonGenerator.h"
#include "CQPToolkit/Net/TwoWayServerConnector.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "CQPToolkit/Alignment/TransmissionHandler.h"
#include "CQPToolkit/Sift/Transmitter.h"
#include "CQPToolkit/PrivacyAmp/PrivacyAmplify.h"
#include "CQPToolkit/ErrorCorrection/ErrorCorrection.h"
#include "CQPToolkit/KeyGen/KeyConverter.h"

namespace cqp
{
    namespace session
    {

        using google::protobuf::Empty;
        using grpc::Status;
        using grpc::ClientContext;

        DummyAliceController::DummyAliceController(std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey) :
            SessionController(creds)
        {
            photonGen.reset(new sim::DummyTransmitter(rng));
            alignment.reset(new align::TransmissionHandler());
            sifter.reset(new sift::Transmitter());
            ec.reset(new ec::ErrorCorrection());
            privacy.reset(new privacy::PrivacyAmplify());
            keyConverter.reset(new keygen::KeyConverter(bytesPerKey));

            // attach each stage to the next in the chain
            photonGen->Attach(alignment.get());
            alignment->Attach(sifter.get());
            sifter->Attach(ec.get());
            ec->Attach(privacy.get());
            privacy->Attach(keyConverter.get());
        } // DummyAliceController

        DummyAliceController::~DummyAliceController()
        {
            EndSession();
            // disconnect the chain
            photonGen->Detatch();
            alignment->Detatch();
            sifter->Detatch();
            ec->Detatch();
            privacy->Detatch();
        } // ~DummyAliceController

        void DummyAliceController::DoWork()
        {
            google::protobuf::Timestamp detectorRequest;
            Empty detectorResponse;

            if(detector && photonGen)
            {
                photonGen->StartFrame();

                grpc::Status detectorStatus;

                /* scope */
                {
                    ClientContext ctx;
                    // tell the remote detector that we are starting to send photons
                    Status detectorStatus = LogStatus(
                                                detector->StartDetecting(&ctx, detectorRequest, &detectorResponse));
                }/* scope */

                if(detectorStatus.ok())
                {
                    // simulate photon transmission
                    photonGen->Fire();

                    ClientContext ctx;
                    // tell the remote detector that we have finished sending photons
                    detectorStatus = LogStatus(
                                         detector->StopDetecting(&ctx, detectorRequest, &detectorResponse));
                } // if detectorStatus.ok()

                // notify that the frame has been sent
                photonGen->EndFrame();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } // if detector && photonGen
            else
            {
                Stop();
                LOGERROR("Setup incomplete");
            } // else
        } // DoWork

        void DummyAliceController::ConnectRemoteChain()
        {
            // connect each stage to it's remote partner
            detector = remote::IDetector::NewStub(twoWayComm->GetClient());
            photonGen->Connect(twoWayComm->GetClient());
            sifter->Connect(twoWayComm->GetClient());

        } // ConnectRemoteChain

        Status DummyAliceController::StartSession(const remote::OpticalParameters& params)
        {
            // stession is being started locally
            Status result = SessionController::StartSession(params);
            if(result.ok() && !IsRunning())
            {
                ConnectRemoteChain();
                // start sending frames
                Start();
            }
            return result;
        } // StartSession

        void DummyAliceController::EndSession()
        {
            SessionController::EndSession();

            // wait for the transmitter to stop
            Stop(true);

            photonGen->Disconnect();
            sifter->Disconnect();
            detector = nullptr;
        } // EndSession

        IKeyPublisher* DummyAliceController::GetKeyPublisher()
        {
            return keyConverter.get();
        }

        std::vector<stats::StatCollection*> DummyAliceController::GetStats() const
        {
            return
            {
                &privacy->stats,
                &photonGen->stats,
                &alignment->stats,
                &sifter->stats,
                &ec->stats
            };
        } // GetKeyPublisher

        Status DummyAliceController::SessionStarting(grpc::ServerContext* context, const remote::SessionDetails* request, Empty* response)
        {
            // session is being started remotely
            Status result = SessionController::SessionStarting(context, request, response);
            if(result.ok())
            {
                ConnectRemoteChain();
                // start sending frames
                Start();
            }
            return result;
        } // SessionStarting

        grpc::Status DummyAliceController::SessionEnding(grpc::ServerContext* context, const Empty* request, Empty* response)
        {
            Status result = SessionController::SessionEnding(context, request, response);
            if(result.ok())
            {
                Stop();
            }
            return result;
        } // SessionEnding

        void DummyAliceController::RegisterServices(grpc::ServerBuilder& builder)
        {
            builder.RegisterService(ec.get());
            builder.RegisterService(privacy.get());
            builder.RegisterService(alignment.get());
            // ^^^ Add new services here ^^^ //
        } // RegisterServices

    } // namespace session
} // namespace cqp

