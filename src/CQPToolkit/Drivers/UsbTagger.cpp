/*!
* @file
* @brief CQP Toolkit - Usb Tagger
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "CQPToolkit/Drivers/UsbTagger.h"
#include <libusb-1.0/libusb.h>       // for libusb_exit, libusb_free_device_...
#include "Algorithms/Logging/Logger.h"  // for LOGDEBUG, LOGERROR, LOGTRACE
#include "CQPToolkit/Drivers/Usb.h"
#include "CQPToolkit/Drivers/Serial.h"
#include "Algorithms/Util/DataFile.h"
#include "Algorithms/Util/ProcessingQueue.h"
#include "Algorithms/Util/Threading.h"

namespace cqp
{
    using namespace std;

    class UsbTagger::DataPusher
    {
    public:
        using DataBlockPtr = std::unique_ptr<DataBlock>;

        DataPusher(Usb& device, const std::vector<Qubit>& channelMappings);

        ~DataPusher();

        void Start(const std::chrono::high_resolution_clock::time_point& epoc, IDetectionEventCallback* newListener);

        void ReadData();

        void ReadDataAsync(std::unique_ptr<DataBlock> data);

        void ConvertData();

        void Stop();

        DataBlockPtr GetBuffer();

        void ReturnBuffer(DataBlockPtr buffer);

    protected:
        std::mutex unusedQueueMutex;
        std::mutex processingQueueMutex;
        std::condition_variable dataReadyCv;
        std::unique_ptr<ProtocolDetectionReport> report;
        bool stopProcessing = false;
        std::thread reader;
        std::thread processor;
        std::queue<DataBlockPtr> unusedBuffers;
        std::queue<DataBlockPtr> processingQueue;
        SequenceNumber frame = 1;
        ::libusb_transfer* activeTransfer = nullptr;

        IDetectionEventCallback* listener = nullptr;
        Usb& device;
        const std::vector<Qubit>& channelMappings;
    };

    UsbTagger::UsbTagger(const string& controlName, const string& usbSerialNumber)
    {
        if(controlName.empty())
        {
            SerialList devices;
            Serial::Detect(devices, true);
            if(devices.empty())
            {
                LOGERROR("No serail device found");
            } else {
                configPort = move(devices[0]);
            }
        } else {
            configPort = std::make_unique<Serial>(controlName, myBaudRate);
        }

        dataPort = Usb::Detect(usbVID, usbPID, usbSerialNumber);

    }

    UsbTagger::UsbTagger(std::unique_ptr<Serial> controlDev, std::unique_ptr<Usb> dataDev) :
        configPort{move(controlDev)},
        dataPort{move(dataDev)},
        dataPusher {make_unique<DataPusher>(*dataPort, channelMappings)}
    {

    }

    UsbTagger::~UsbTagger() = default;

    grpc::Status UsbTagger::StartDetecting(grpc::ServerContext*, const google::protobuf::Timestamp* request, google::protobuf::Empty*)
    {
        using std::chrono::high_resolution_clock;
        grpc::Status result;
        if(configPort)
        {
            // TODO: convert the incomming timestamp into an epoc
            // Start reading data from the usb port
            dataPusher->Start(high_resolution_clock::now(), listener);
            // Start the detector
            configPort->Write('R');
        } else {
            result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Invalid serial device");
        }
        return result;
    }

    grpc::Status UsbTagger::StopDetecting(grpc::ServerContext*, const google::protobuf::Timestamp* request, google::protobuf::Empty*)
    {
        grpc::Status result;
        if(configPort)
        {
            configPort->Write('S');
            dataPusher->Stop();
        } else {
            result = grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Invalid serial device");
        }
        return result;
    }

    bool UsbTagger::Initialise()
    {
        bool result = true;

        if (configPort != nullptr)
        {
            const char InitSeq[] = { 'W', 'S' };
            // predefined command sequence for initialisation
            for (char cmd : InitSeq)
            {
                result &= configPort->Write(cmd);
                this_thread::sleep_for(chrono::seconds(1));
            }
        }

        return result;
    }

    URI UsbTagger::GetAddress()
    {
        URI result;
        std::string hostString = fs::BaseName(configPort->GetAddress().GetPath());
        for(auto element : dataPort->GetPortNumbers())
        {
            hostString += "-" + std::to_string(element);
        }
        result.SetHost(hostString);
        result.SetParameter(Parameters::serial, configPort->GetAddress().GetPath());
        result.SetParameter(Parameters::usbserial, dataPort->GetSerialNumber());

        return result;
    }

    void UsbTagger::SetChannelMappings(const std::vector<Qubit>& mapping)
    {
        channelMappings = mapping;
    }

    UsbTagger::DataPusher::DataPusher(Usb& device, const std::vector<Qubit>& channelMappings)  :
        device{device},
        channelMappings{channelMappings}
    {

    }

    UsbTagger::DataPusher::~DataPusher()
    {
        Stop();
    }

    void UsbTagger::DataPusher::Start(const chrono::system_clock::time_point& epoc, IDetectionEventCallback* newListener)
    {
        listener = newListener;
        report = std::make_unique<ProtocolDetectionReport>();
        report->epoc = epoc;
        report->frame = frame;
        reader = thread(&DataPusher::ReadData, this);
        // try and give the reader higher priority to reduce the risk of buffer overflows
        // this will fail if we dont have the rights
        threads::SetPriority(reader, -1);

        processor = std::thread(&DataPusher::ConvertData, this);
        // make the processing thread nicer than the reading thread
        threads::SetPriority(processor, 1);

        // get a buffer for the data
        activeTransfer = device.StartReadingBulk<DataPusher>(bulkReadRequest, GetBuffer(), &DataPusher::ReadDataAsync, this);
    }

    void UsbTagger::DataPusher::ReadData()
    {
        using namespace std;

        while(!stopProcessing)
        {
            // get a buffer for the data
            auto data = GetBuffer();
            // read from the device
            device.ReadBulk(*data, bulkReadRequest);

            {
                unique_lock<mutex> lock(processingQueueMutex);
                processingQueue.push(move(data));
            }
            // trigger the converter to process it
            dataReadyCv.notify_one();
        }
    }

    void UsbTagger::DataPusher::ReadDataAsync(std::unique_ptr<DataBlock> data)
    {

        {
            unique_lock<mutex> lock(processingQueueMutex);
            processingQueue.push(move(data));
        }
        // trigger the converter to process it
        dataReadyCv.notify_one();

        if(!stopProcessing)
        {
            activeTransfer = device.StartReadingBulk(bulkReadRequest, GetBuffer(), &DataPusher::ReadDataAsync, this);
        }
    }

    void UsbTagger::DataPusher::ConvertData()
    {
        using NoxReport = fs::DataFile::NoxReport;
        NoxReport devReport;
        DataBlockPtr data;

        while(!stopProcessing)
        {
            {
                unique_lock<mutex> lock(processingQueueMutex);
                dataReadyCv.wait(lock, [&](){
                    return !processingQueue.empty() || stopProcessing;
                });

                if(!stopProcessing)
                {
                    data = move(processingQueue.front());
                    processingQueue.pop();
                }
            }

            if(data)
            {
                if(data->size() % NoxReport::messageBytes == 0)
                {
                    const auto numDetections = data->size() / NoxReport::messageBytes;
                    // reserve space for the new items
                    report->detections.reserve(report->detections.size() + numDetections);

                    for(auto offset = 0u; offset < data->size(); offset += NoxReport::messageBytes)
                    {
                        // read the report
                        if(devReport.LoadRaw(&(*data)[offset]))
                        {
                            if(devReport.messageType == NoxReport::MessageType::Detection &&
                               devReport.detection.channel < channelMappings.size())
                            {
                                report->detections.push_back({devReport.GetTime(), channelMappings[devReport.detection.channel]});
                            }
                        } // if report valid

                    } // for each message
                } else
                {
                    LOGERROR("Data size invalid");
                }

                // but the buffer back onto the queue for the reader to use
                data->clear();
                ReturnBuffer(move(data));
            }
        }// while !stopProcessing

        if(listener)
        {
            listener->OnPhotonReport(move(report));
        }
    }

    void UsbTagger::DataPusher::Stop()
    {
        stopProcessing = true;
        if(activeTransfer != nullptr)
        {
            device.CancelTransfer(activeTransfer);
        }
        dataReadyCv.notify_all();

        if(reader.joinable())
        {
            reader.join();
        }
        if(processor.joinable())
        {
            processor.join();
        }
        frame++;
    }

    UsbTagger::DataPusher::DataBlockPtr UsbTagger::DataPusher::GetBuffer()
    {
        using namespace std;
        DataBlockPtr result;

        unique_lock<mutex> lock(unusedQueueMutex);
        if(unusedBuffers.empty())
        {
            result = make_unique<DataBlock>();
        } else {
            result = move(unusedBuffers.front());
            unusedBuffers.pop();
        }

        return result;
    }

    void UsbTagger::DataPusher::ReturnBuffer(UsbTagger::DataPusher::DataBlockPtr buffer)
    {
        using namespace std;
        unique_lock<mutex> lock(unusedQueueMutex);
        unusedBuffers.push(move(buffer));
    }

} // namespace cqp
