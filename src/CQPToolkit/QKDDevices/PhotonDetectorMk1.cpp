/*!
* @file
* @brief CQP Toolkit - Photon Detection
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/QKDDevices/PhotonDetectorMk1.h"
#include <chrono>                    // for seconds
#include <thread>                    // for sleep_for
#include "Algorithms/Logging/Logger.h"  // for LOGINFO, LOGERROR
#include "Drivers/Serial.h"          // for Serial
#include "Drivers/UsbTagger.h"       // for UsbTagger, UsbTaggerList
#include "Drivers/Usb.h"       // for UsbTagger, UsbTaggerList
#include "CQPToolkit/Alignment/DetectionReciever.h"
#include "ErrorCorrection/ErrorCorrection.h"
#include "PrivacyAmp/PrivacyAmplify.h"
#include "KeyGen/KeyConverter.h"
#include "CQPToolkit/Session/SessionController.h"
#include "CQPToolkit/Statistics/ReportServer.h"

namespace cqp
{
    using namespace std;

    /// The name of the device
    const std::string PhotonDetectorMk1::DriverName = "Mk1Tagger";


    class PhotonDetectorMk1::ProcessingChain
    {
    public:
        ProcessingChain()
        {
            using namespace std;
            align = make_shared<align::DetectionReciever>();
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

        shared_ptr<align::DetectionReciever> align;
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
            remotes.push_back(align);
            remotes.push_back(ec);
            return remotes;
        }

        void RegisterServices(grpc::ServerBuilder& builder)
        {
            builder.RegisterService(ec.get());
            builder.RegisterService(privacy.get());
            builder.RegisterService(reportServer.get()); // allow external clients to get stats

        }
    };

    PhotonDetectorMk1::PhotonDetectorMk1(std::shared_ptr<grpc::ChannelCredentials> creds, const std::string& controlName, const std::string& usbSerialNumber) :
        processing{std::make_unique<ProcessingChain>()},
        driver{std::make_shared<UsbTagger>(controlName, usbSerialNumber)}
    {
        // create the session controller
        sessionController = std::make_unique<session::SessionController>(creds, processing->GetRemotes(), processing->reportServer);
        // link the output of the photon generator to the post processing
        driver->Attach(processing->align.get());
    }

    PhotonDetectorMk1::PhotonDetectorMk1(std::shared_ptr<grpc::ChannelCredentials> creds, std::unique_ptr<Serial> serialDev, std::unique_ptr<Usb> usbDev) :
        processing{std::make_unique<ProcessingChain>()},
        driver{std::make_shared<UsbTagger>(move(serialDev), move(usbDev))}
    {
        // create the session controller
        sessionController = std::make_unique<session::SessionController>(creds, processing->GetRemotes(),
                            processing->reportServer);
        // link the output of the photon generator to the post processing
        driver->Attach(processing->align.get());
    }

    PhotonDetectorMk1::~PhotonDetectorMk1() = default;

    string PhotonDetectorMk1::GetDriverName() const
    {
        return DriverName;
    }

    bool PhotonDetectorMk1::Initialise(const remote::SessionDetails& sessionDetails)
    {
        bool result = driver->Initialise();

        return result;
    }

    URI PhotonDetectorMk1::GetAddress() const
    {
        URI result = driver->GetAddress();
        result.SetScheme(DriverName);
        result.SetParameter(IQKDDevice::Parameters::side, IQKDDevice::Parameters::SideValues::bob);
        result.SetParameter(IQKDDevice::Parameters::keybytes, "16");
        return result;
    }

    ISessionController* PhotonDetectorMk1::GetSessionController()
    {
        return sessionController.get();
    }

    cqp::KeyPublisher* PhotonDetectorMk1::GetKeyPublisher()
    {
        return processing->keyConverter.get();
    }

    cqp::remote::DeviceConfig PhotonDetectorMk1::GetDeviceDetails()
    {
        remote::DeviceConfig result;
        // TODO
        //result.set_id(myPortName);
        result.set_side(remote::Side_Type::Side_Type_Bob);
        result.set_kind(DriverName);
        return result;
    }

    void PhotonDetectorMk1::RegisterServices(grpc::ServerBuilder& builder)
    {
        processing->RegisterServices(builder);
        builder.RegisterService(driver.get());
    }

    void PhotonDetectorMk1::SetInitialKey(std::unique_ptr<PSK> initialKey)
    {
        // TODO
    }
} // namespace cqp

