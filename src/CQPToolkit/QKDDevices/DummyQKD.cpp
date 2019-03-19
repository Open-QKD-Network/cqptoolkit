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

    DummyQKD::DummyQKD(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds)
    {
        const URI addrUri(address);
        if(addrUri.GetScheme() != DriverName)
        {
            LOGWARN("Driver name " + addrUri.GetScheme() + " doesnt match this driver (" + DriverName + ")");
        }

        for(const auto& param : addrUri.GetQueryParameters())
        {
            if(param.first == Parameters::switchPort)
            {
                config.set_switchport(param.second);
            }
            else if(param.first == Parameters::side)
            {
                if(param.second == Parameters::SideValues::alice)
                {
                    config.set_side(remote::Side_Type::Side_Type_Alice);
                }
                else if(param.second == Parameters::SideValues::bob)
                {
                    config.set_side(remote::Side_Type::Side_Type_Bob);
                }
                else if(param.second == Parameters::SideValues::any)
                {
                    config.set_side(remote::Side_Type::Side_Type_Any);
                }
            }
            else if(param.first == Parameters::switchName)
            {
                config.set_switchname(param.second);
            }
            else if(param.first == Parameters::keybytes)
            {
                try
                {
                    config.set_bytesperkey(static_cast<uint32_t>(std::stoi(param.second)));
                }
                catch (const std::exception& e)
                {
                    LOGERROR(e.what());
                }
            }
            else
            {
                LOGWARN("Unknown parameter: " + param.first);
            }
        }
        processing = std::make_unique<ProcessingChain>(creds, &rng, config.side());

        // reset any values that cant be changed
        config.set_kind(DriverName);
        if(config.id().empty())
        {
            config.set_id(DeviceUtils::GetDeviceIdentifier(GetAddress()));
        }
    }

    DummyQKD::DummyQKD(const remote::DeviceConfig& initialConfig, std::shared_ptr<grpc::ChannelCredentials> creds):
        processing{std::make_unique<ProcessingChain>(creds, &rng, initialConfig.side())},
        config{initialConfig}
    {
        // reset any values that cant be changed
        config.set_kind(DriverName);
        if(config.id().empty())
        {
            config.set_id(DeviceUtils::GetDeviceIdentifier(GetAddress()));
        }
    }

    void DummyQKD::SetInitialKey(std::unique_ptr<PSK> initialKey)
    {
        // TODO
    }

    DummyQKD::~DummyQKD() = default;

    std::string cqp::DummyQKD::GetDriverName() const
    {
        return DriverName;
    }

    URI cqp::DummyQKD::GetAddress() const
    {
        return DeviceUtils::ConfigToUri(config);

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
        config.set_sessionaddress(processing->controller->GetConnectionAddress());
        return config;
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

