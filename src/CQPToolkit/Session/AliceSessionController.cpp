/*!
* @file
* @brief SessionController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/2/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Session/AliceSessionController.h"
#include "CQPToolkit/Util/GrpcLogger.h"
#include "QKDInterfaces/IDetector.grpc.pb.h"
#include "CQPToolkit/Session/TwoWayServerConnector.h"

namespace cqp {
    namespace session {

        using google::protobuf::Empty;
        using grpc::Status;
        using grpc::ClientContext;

        const int AliceSessionController::threadPriority;

        AliceSessionController::AliceSessionController(std::shared_ptr<grpc::ChannelCredentials> creds,
                                                       const Services& services,
                                                       const RemoteCommsList& remotes, std::shared_ptr<IPhotonGenerator> source,
                                                       std::shared_ptr<stats::ReportServer> theReportServer) :
            SessionController (creds, services, remotes, theReportServer),
            photonSource{source}
        {

        }

        grpc::Status AliceSessionController::StartSession()
        {
            // the local system is starting the session
            auto result = SessionController::StartSession();
            if(result.ok() && photonSource)
            {
                Start(threadPriority);
            }
            return result;
        }

        void AliceSessionController::EndSession()
        {
            // the local system is stopping the session
            // wait for the transmitter to stop
            Stop(true);

            SessionController::EndSession();
        }

        grpc::Status AliceSessionController::SessionStarting(grpc::ServerContext* ctx, const remote::SessionDetailsFrom* request, google::protobuf::Empty* response)
        {
            // The session has been started remotly
            auto result = SessionController::SessionStarting(ctx, request, response);
            if(result.ok() && photonSource)
            {
                Start(threadPriority);
            }
            return result;
        }

        grpc::Status AliceSessionController::SessionEnding(grpc::ServerContext* ctx, const google::protobuf::Empty* request, google::protobuf::Empty* response)
        {
            // The session has been stopped remotly
            // wait for the transmitter to stop
            Stop(true);

            return SessionController::SessionEnding(ctx, request, response);
        }

        void AliceSessionController::DoWork()
        {
            google::protobuf::Timestamp detectorRequest;
            Empty detectorResponse;
            auto detector = remote::IDetector::NewStub(twoWayComm->GetClient());

            while(!ShouldStop())
            {
                if(detector && photonSource)
                {
                    // The photon source will negotiate with the detector
                    photonSource->StartFrame();

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
                        photonSource->Fire();

                        ClientContext ctx;
                        // tell the remote detector that we have finished sending photons
                        detectorStatus = LogStatus(
                                             detector->StopDetecting(&ctx, detectorRequest, &detectorResponse));
                    } // if detectorStatus.ok()

                    // notify that the frame has been sent
                    photonSource->EndFrame();
                } // if detector && photonGen
                else
                {
                    Stop();
                    LOGERROR("Setup incomplete");
                } // else
            } // while
        }
    } // namespace session
} // namespace cqp

