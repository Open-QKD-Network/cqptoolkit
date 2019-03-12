/*!
* @file
* @brief DummyQKD
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 30/1/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DummyQKD.h"
#include <utility>                                    // for move
#include "CQPToolkit/Session/AliceSessionController.h"
#include "Interfaces/IQKDDevice.h"                    // for IQKDDevice, IQK...
#include "Session/SessionController.h"                // for SessionController
#include "Algorithms/Logging/Logger.h"                              // for LOGERROR
#include "Alignment/NullAlignment.h"
#include "ErrorCorrection/ErrorCorrection.h"
#include "Sift/Transmitter.h"
#include "Sift/Receiver.h"
#include "PrivacyAmp/PrivacyAmplify.h"
#include "KeyGen/KeyConverter.h"
#include "CQPToolkit/Simulation/DummyTransmitter.h"
#include "CQPToolkit/Simulation/DummyTimeTagger.h"
#include "CQPToolkit/Statistics/ReportServer.h"
#include "DeviceUtils.h"

namespace cqp
{
    class ISessionController;

    constexpr const char* DummyQKD::DriverName;

    class DummyQKD::ProcessingChain
    {
    public:
        ProcessingChain(std::shared_ptr<grpc::ChannelCredentials> creds,
                        IRandom* rng, remote::Side::Type side) :
            alignment(std::make_shared<align::NullAlignment>()),
          ec(std::make_shared<ec::ErrorCorrection>()),
          privacy(std::make_shared<privacy::PrivacyAmplify>()),
          keyConverter(std::make_shared<keygen::KeyConverter>()),
          reportServer(std::make_shared<stats::ReportServer>())
        {
            using namespace std;
            session::SessionController::RemoteCommsList remotes;
            session::SessionController::Services services;
            remotes.push_back(alignment);
            services.push_back(reportServer.get());

            // create the controller for this device
            switch (side)
            {
            case remote::Side::Alice:
                {
                    photonSource = make_shared<sim::DummyTransmitter>(rng);
                    photonSource->Attach(alignment.get());
                    remotes.push_back(photonSource);
                    photonSource->stats.Add(reportServer.get());

                    auto transmitter = std::make_shared<sift::Transmitter>();
                    sifter = transmitter;
                    remotes.push_back(transmitter);

                    controller = make_shared<session::AliceSessionController>(creds, services, remotes, photonSource, reportServer);
                }

                break;
            case remote::Side::Bob:
                {
                    timeTagger = make_shared<sim::DummyTimeTagger>(rng);
                    timeTagger->Attach(alignment.get());
                    services.push_back(static_cast<remote::IDetector::Service*>(timeTagger.get()));
                    services.push_back(static_cast<remote::IPhotonSim::Service*>(timeTagger.get()));

                    timeTagger->stats.Add(reportServer.get());

                    auto receiver = std::make_shared<sift::Receiver>();
                    sifter = receiver;
                    services.push_back(receiver.get());

                    controller = make_shared<session::SessionController>(creds, services, remotes, reportServer);
                }
                break;
            default:
                LOGERROR("Invalid device side");
                break;
            }

            alignment->Attach(sifter.get());
            sifter->Attach(ec.get());
            ec->Attach(privacy.get());
            privacy->Attach(keyConverter.get());

            // send stats to our report server
            sifter->stats.Add(reportServer.get());
            ec->stats.Add(reportServer.get());
            privacy->stats.Add(reportServer.get());
        }
        /// aligns detections
        std::shared_ptr<align::NullAlignment> alignment;
        /// sifts alignments
        std::shared_ptr<sift::SiftBase> sifter;
        /// error corrects sifted data
        std::shared_ptr<ec::ErrorCorrection> ec;
        /// verify corrected data
        std::shared_ptr<privacy::PrivacyAmplify> privacy;
        /// prepare keys for the keystore
        std::shared_ptr<keygen::KeyConverter> keyConverter;

        /// Produces photons when being alice
        std::shared_ptr<sim::DummyTransmitter> photonSource = nullptr;

        /// detects photons
        std::shared_ptr<sim::DummyTimeTagger> timeTagger = nullptr;

        std::shared_ptr<session::SessionController> controller = nullptr;

        std::shared_ptr<stats::ReportServer> reportServer;
    };

    DummyQKD::DummyQKD(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey) :
        DummyQKD(DeviceUtils::GetSide(address), creds, bytesPerKey)
    {
        myAddress = address;
    }

    DummyQKD::~DummyQKD() = default;

    DummyQKD::DummyQKD(const remote::Side::Type & side, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey):
        processing{std::make_unique<ProcessingChain>(creds, &rng, side)}
    {
        myAddress = std::string(DriverName) + ":///?side=";
        switch (side)
        {
        case remote::Side::Alice:
            myAddress += "alice";
            break;
        case remote::Side::Bob:
            myAddress += "bob";
            break;
        default:
            LOGERROR("Invalid device side");
            break;
        }
    }

    std::string cqp::DummyQKD::GetDriverName() const
    {
        return DriverName;
    }

    URI cqp::DummyQKD::GetAddress() const
    {
        return myAddress;
    }

    bool cqp::DummyQKD::Initialise(const remote::SessionDetails& sessionDetails)
    {
        return true;
    }

    ISessionController* DummyQKD::GetSessionController()
    {
        return processing->controller.get();
    }

    remote::DeviceConfig DummyQKD::GetDeviceDetails()
    {
        remote::DeviceConfig result;
        URI addrUri(myAddress);

        result.set_id(DeviceUtils::GetDeviceIdentifier(addrUri));
        std::string sideName;
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::side, sideName);
        result.set_side(DeviceUtils::GetSide(addrUri));
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchName, *result.mutable_switchname());
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchPort, *result.mutable_switchport());
        result.set_kind(addrUri.GetScheme());

        return result;
    }

    stats::IStatsPublisher* DummyQKD::GetStatsPublisher()
    {
        return processing->reportServer.get();
    }

    KeyPublisher* DummyQKD::GetKeyPublisher()
    {
        return processing->keyConverter.get();
    }

} // namespace cqp

