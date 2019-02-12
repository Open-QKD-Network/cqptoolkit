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
#include "CQPToolkit/QKDDevices/DeviceFactory.h"         // for DeviceFactory
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
          keyConverter(std::make_shared<keygen::KeyConverter>())
        {
            using namespace std;
            session::SessionController::RemoteCommsList remotes;
            session::SessionController::Services services;
            remotes.push_back(alignment);

            // create the controller for this device
            switch (side)
            {
            case remote::Side::Alice:
                {
                    photonSource = make_shared<sim::DummyTransmitter>(rng);
                    photonSource->Attach(alignment.get());
                    remotes.push_back(photonSource);

                    auto transmitter = std::make_shared<sift::Transmitter>();
                    sifter = transmitter;
                    remotes.push_back(transmitter);

                    controller = make_shared<session::AliceSessionController>(creds, services, remotes, photonSource);
                }

                break;
            case remote::Side::Bob:
                {
                    timeTagger = make_shared<sim::DummyTimeTagger>(rng);
                    timeTagger->Attach(alignment.get());
                    services.push_back(static_cast<remote::IDetector::Service*>(timeTagger.get()));
                    services.push_back(static_cast<remote::IPhotonSim::Service*>(timeTagger.get()));

                    auto receiver = std::make_shared<sift::Receiver>();
                    sifter = receiver;
                    services.push_back(receiver.get());

                    controller = make_shared<session::SessionController>(creds, services, remotes);
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

    };

    void DummyQKD::RegisterWithFactory()
    {
        // tell the factory how to create a DummyQKD by specifying a lambda function
        // this driver can do both sides
        for(auto side : {remote::Side::Alice, remote::Side::Bob})
        {
            DeviceFactory::RegisterDriver(DriverName, side, [](const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey)
            {
                return std::make_shared<DummyQKD>(address, creds, bytesPerKey);
            });
        }
    }

    DummyQKD::DummyQKD(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, size_t bytesPerKey) :
        DummyQKD(DeviceFactory::GetSide(URI(address)), creds, bytesPerKey)
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

    bool cqp::DummyQKD::Initialise()
    {
        return true;
    }

    ISessionController* DummyQKD::GetSessionController()
    {
        return processing->controller.get();
    }

    remote::Device DummyQKD::GetDeviceDetails()
    {
        remote::Device result;
        URI addrUri(myAddress);

        result.set_id(DeviceFactory::GetDeviceIdentifier(addrUri));
        std::string sideName;
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::side, sideName);
        result.set_side(DeviceFactory::GetSide(addrUri));
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchName, *result.mutable_switchname());
        addrUri.GetFirstParameter(IQKDDevice::Parmeters::switchPort, *result.mutable_switchport());
        result.set_kind(addrUri.GetScheme());

        return result;
    }

    std::vector<stats::StatCollection*> DummyQKD::GetStats()
    {
        std::vector<stats::StatCollection*> results;
        results.push_back(&processing->ec->stats);
        results.push_back(&processing->privacy->stats);
        results.push_back(&processing->sifter->stats);

        if(processing->timeTagger)
        {
            results.push_back(&processing->timeTagger->stats);
        }
        if(processing->photonSource)
        {
            results.push_back(&processing->photonSource->stats);
        }
        return results;
    }

    IKeyPublisher* DummyQKD::GetKeyPublisher()
    {
        return processing->keyConverter.get();
    }

} // namespace cqp

