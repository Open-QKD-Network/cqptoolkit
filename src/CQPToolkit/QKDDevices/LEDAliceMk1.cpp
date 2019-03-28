/*!
* @file
* @brief CQP Toolkit - Cheap and cheerful LED driver Mk1
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 22 Sep 2016
* @author David Lowndes <david.lowndes@bristol.ac.uk>
*/
#include "LEDAliceMk1.h"
#include "Algorithms/Logging/Logger.h"            // for LOGERROR
#include "CQPToolkit/Alignment/TransmissionHandler.h"
#include "ErrorCorrection/ErrorCorrection.h"
#include "PrivacyAmp/PrivacyAmplify.h"
#include "KeyGen/KeyConverter.h"
#include "CQPToolkit/Session/AliceSessionController.h"
#include "CQPToolkit/Drivers/LEDDriver.h"
#include "CQPToolkit/Drivers/Usb.h"
#include "CQPToolkit/Statistics/ReportServer.h"
#include "QKDInterfaces/Device.pb.h"

namespace cqp
{
    class ISessionController;
}

namespace cqp
{
    using std::shared_ptr;
    using std::unique_ptr;

    const std::string LEDAliceMk1::DriverName = "LEDAliceMk1";

    class LEDAliceMk1::ProcessingChain
    {
    public:
        ProcessingChain()
        {
            using namespace std;
            align = make_shared<align::TransmissionHandler>();
            ec = make_shared<ec::ErrorCorrection>();
            privacy = make_shared<privacy::PrivacyAmplify>();
            reportServer = make_shared<stats::ReportServer>();

            align->Attach(ec.get());
            ec->Attach(privacy.get());
            privacy->Attach(keyConverter.get());

            // send stats to our report server
            align->stats.Add(reportServer.get());
            ec->stats.Add(reportServer.get());
            privacy->stats.Add(reportServer.get());
        }

        shared_ptr<align::TransmissionHandler> align;
        /// error corrects sifted data
        shared_ptr<ec::ErrorCorrection> ec;
        /// verify corrected data
        shared_ptr<privacy::PrivacyAmplify> privacy;
        /// prepare keys for the keystore
        shared_ptr<keygen::KeyConverter> keyConverter;

        shared_ptr<stats::ReportServer> reportServer;

        session::SessionController::RemoteCommsList GetRemotes() const
        {
            session::SessionController::RemoteCommsList remotes;
            return remotes;
        }

        void RegisterServices(grpc::ServerBuilder& builder)
        {
            builder.RegisterService(align.get());
            builder.RegisterService(ec.get());
            builder.RegisterService(privacy.get());
            builder.RegisterService(reportServer.get());
        }
    };

    LEDAliceMk1::LEDAliceMk1(std::shared_ptr<grpc::ChannelCredentials> creds,
                             const std::string& controlName, const std::string& usbSerialNumber) :
        processing{std::make_unique<ProcessingChain>()},
        driver{std::make_shared<LEDDriver>(&rng, controlName, usbSerialNumber)}
    {
        // create the session controller
        sessionController = std::make_unique<session::AliceSessionController>(creds, processing->GetRemotes(), driver, processing->reportServer);
        // link the output of the photon generator to the post processing
        driver->Attach(processing->align.get());
    }

    LEDAliceMk1::LEDAliceMk1(std::shared_ptr<grpc::ChannelCredentials> creds,
                             std::unique_ptr<Serial> controlPort, std::unique_ptr<Usb> dataPort):
        processing{std::make_unique<ProcessingChain>()},
        driver{std::make_shared<LEDDriver>(&rng, move(controlPort), move(dataPort))}
    {
        // create the session controller
        sessionController = std::make_unique<session::AliceSessionController>(creds, processing->GetRemotes(), driver, processing->reportServer);
        // link the output of the photon generator to the post processing
        driver->Attach(processing->align.get());
    }

    void LEDAliceMk1::SetInitialKey(std::unique_ptr<PSK> initialKey)
    {
        // TODO
    }

    LEDAliceMk1::~LEDAliceMk1() = default;

    std::string LEDAliceMk1::GetDriverName() const
    {
        return DriverName;
    }

    remote::DeviceConfig LEDAliceMk1::GetDeviceDetails()
    {
        remote::DeviceConfig result;
        // TODO
        //result.set_id(myPortName);
        result.set_side(remote::Side_Type::Side_Type_Alice);
        result.set_kind(DriverName);
        return result;
    }

    URI LEDAliceMk1::GetAddress() const
    {
        URI result = driver->GetAddress();
        result.SetScheme(DriverName);
        result.SetParameter(IQKDDevice::Parameters::side, IQKDDevice::Parameters::SideValues::alice);
        result.SetParameter(IQKDDevice::Parameters::keybytes, "16");
        return result;
    }

    bool LEDAliceMk1::Initialise(const remote::SessionDetails& sessionDetails)
    {
        bool result = driver->Initialise();
        return result;
    }

    void LEDAliceMk1::RegisterServices(grpc::ServerBuilder &builder)
    {
        processing->RegisterServices(builder);
    }

    ISessionController* LEDAliceMk1::GetSessionController()
    {
        return sessionController.get();
    }

    KeyPublisher*cqp::LEDAliceMk1::GetKeyPublisher()
    {
        return processing->keyConverter.get();
    }

}




