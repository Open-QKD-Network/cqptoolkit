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
#include "Sift/Receiver.h"
#include "Sift/Verifier.h"
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
            remotes.push_back(alignment);

            // create the controller for this device
            switch (side)
            {
            case remote::Side::Alice:
            {
                photonSource = make_shared<sim::DummyTransmitter>(rng, chrono::microseconds(10));
                siftVerifier = std::make_shared<sift::Verifier>();
                // build the pipeline
                photonSource->Attach(siftVerifier.get());
                siftVerifier->Attach(ec.get());

                // send stats to our report server
                photonSource->stats.Add(reportServer.get());
                siftVerifier->stats.Add(reportServer.get());
                // let classes get notified when we are connected
                remotes.push_back(photonSource);
                remotes.push_back(siftVerifier);

            }

            break;
            case remote::Side::Bob:
            {
                timeTagger = make_shared<sim::DummyTimeTagger>(rng);
                siftReceiver = std::make_shared<sift::Receiver>();
                // build the pipeline
                timeTagger->Attach(siftReceiver.get());
                siftReceiver->Attach(ec.get());

                // send stats to our report server
                timeTagger->stats.Add(reportServer.get());
                siftReceiver->stats.Add(reportServer.get());

                // let classes get notified when we are connected
                remotes.push_back(timeTagger);
                remotes.push_back(siftVerifier);

            }
            break;
            default:
                LOGERROR("Invalid device side");
                break;
            }

            controller = make_shared<session::SessionController>(creds, remotes, reportServer);

            ec->Attach(privacy.get());
            privacy->Attach(keyConverter.get());

            // send stats to our report server
            ec->stats.Add(reportServer.get());
            privacy->stats.Add(reportServer.get());
        }

        ~ProcessingChain()
        {
            controller->EndSession();

        }

        void RegisterServices(grpc::ServerBuilder& builder)
        {
            builder.RegisterService(reportServer.get());
            if(timeTagger)
            {
                builder.RegisterService(static_cast<remote::IDetector::Service*>(timeTagger.get()));
                builder.RegisterService(static_cast<remote::IPhotonSim::Service*>(timeTagger.get()));
            }
            if(siftVerifier)
            {
                builder.RegisterService(siftVerifier.get());
            }
        }

        /// aligns detections
        std::shared_ptr<align::NullAlignment> alignment;
        /// sifts alignments
        std::shared_ptr<sift::Receiver> siftReceiver;
        /// error corrects sifted data
        std::shared_ptr<ec::ErrorCorrection> ec;
        /// verify corrected data
        std::shared_ptr<privacy::PrivacyAmplify> privacy;
        /// prepare keys for the keystore
        std::shared_ptr<keygen::KeyConverter> keyConverter;

        std::shared_ptr<sift::Verifier> siftVerifier;
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
                std::vector<std::string> ports;
                SplitString(param.second, ports, ",");
                for(const auto& port : ports)
                {
                    config.add_switchport(port);
                }
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
        return config;
    }

    void DummyQKD::RegisterServices(grpc::ServerBuilder &builder)
    {
        processing->RegisterServices(builder);
    }

    KeyPublisher* DummyQKD::GetKeyPublisher()
    {
        return processing->keyConverter.get();
    }

} // namespace cqp

