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
#include "Algorithms/Logging/Logger.h"  // for LOGDEBUG, LOGERROR, LOGTRACE
#include "CQPToolkit/Drivers/Usb.h"
#include "CQPToolkit/Drivers/Serial.h"
#include "Algorithms/Util/DataFile.h"
#include "Algorithms/Util/ProcessingQueue.h"
#include "Algorithms/Util/Threading.h"

namespace cqp
{
    using namespace std;

    /**
     * @brief The UsbTagger::DataPusher class
     * Marshall data from the device and process it into detection reports.
     * This uses threads and asyncronous IO to minimise the chance of buffer overflow
     * @details
     * @startuml
        participant "Usb" as usb
        participant "DataPusher" as dp
        participant "data : queue" as data
        participant "report" as report
        boundary "IDetectionEventCallback" as listener

        [-> dp : Start()
        activate dp
            note over dp
                Setup the report for the listener
            end note
            note over usb, dp
                The usb device will call back when data is ready
            end note
            dp -> usb : StartReadingBulk()
        deactivate dp

        par
            [-\ usb
            activate usb
                usb -> dp : ReadDataAsync()
                activate dp
                    note over dp
                        Extracted raw data is pushed onto a queue
                        and then the next request is issued
                    end note
                    dp -> data : push()
                deactivate dp
            deactivate usb
        else
        [--> dp : ConvertData()
            activate dp
                loop
                    dp -> data : pop()
                    note over dp
                        Convert the data to a report and
                        append it to the list
                    end note
                    dp -> report : append()
                end loop
            deactivate dp
        end par

        [-> dp : Stop()
        activate dp
            note over dp
                Wait for procesing to finish then send
                the report
            end note
            dp -> listener : OnPhotonreport()
        deactivate dp
        @enduml
     */
    class UsbTagger::DataPusher
    {
    public:
        /// managed pointer for data
        using DataBlockPtr = std::unique_ptr<DataBlock>;

        /**
         * @brief DataPusher
         * Create the instance to manage a device
         * @param device The device to read data from
         * @param channelMappings
         */
        DataPusher(Usb& device, const std::vector<Qubit>& channelMappings);

        /// destructor
        ~DataPusher();

        /**
         * @brief Start
         * Start reading detections from the usb device. The full report is sent once the reading has been stopped
         * @param epoc The time which the transmitter started generating
         * @param newListener The receiver of the detection report
         */
        void Start(const std::chrono::high_resolution_clock::time_point& epoc, IDetectionEventCallback* newListener);


        void Stop();

    protected: // members

        void ReadDataAsync(std::unique_ptr<DataBlock> data);

        void ConvertData();

        DataBlockPtr GetBuffer();

        void ReturnBuffer(DataBlockPtr buffer);

    protected: // members
        /// destination for the final report
        IDetectionEventCallback* listener = nullptr;
        /// The device to read
        Usb& device;
        /// how channels are mapped to qubits
        const std::vector<Qubit>& channelMappings;

        std::mutex unusedQueueMutex;
        std::mutex processingQueueMutex;
        std::condition_variable dataReadyCv;
        std::unique_ptr<ProtocolDetectionReport> report;
        bool shutdown = false;
        bool keepReading = true;
        std::thread processor;
        std::queue<DataBlockPtr> unusedBuffers;
        std::queue<DataBlockPtr> processingQueue;
        SequenceNumber frame = 1;
        ::libusb_transfer* activeTransfer = nullptr;

        /// The maximum number of bytes to read in any one bulk read event
        static const int maxBulkRead = 8192;

    };

    // ******* DataPusher methods **************

    UsbTagger::DataPusher::DataPusher(Usb& device, const std::vector<Qubit>& channelMappings)  :
        device{device},
        channelMappings{channelMappings},
        processor{std::thread(&DataPusher::ConvertData, this)}
    {
        // make the processing thread nicer than the reading thread
        threads::SetPriority(processor, 1);

    }

    UsbTagger::DataPusher::~DataPusher()
    {
        shutdown = true;

        dataReadyCv.notify_all();

        if(processor.joinable())
        {
            processor.join();
        }

        // if the usb callback hasn't been called by now we need to cancel it
        if(activeTransfer)
        {
            device.CancelTransfer(activeTransfer);
        }
    }

    void UsbTagger::DataPusher::Start(const chrono::system_clock::time_point& epoc, IDetectionEventCallback* newListener)
    {
        listener = newListener;
        report = std::make_unique<ProtocolDetectionReport>();
        report->epoc = epoc;
        report->frame = frame;
        keepReading = true;

        // get a buffer for the data
        activeTransfer = device.StartReadingBulk<DataPusher>(bulkReadRequest, GetBuffer(), &DataPusher::ReadDataAsync, this);
    }

    void UsbTagger::DataPusher::Stop()
    {
        // tell the read callback to stop
        keepReading = false;

        // wait for all the data to processed
        {/*lock scope*/
            unique_lock<mutex> lock(processingQueueMutex);
            dataReadyCv.wait(lock, [&](){
                return processingQueue.empty();
            });

            if(listener)
            {
                // send the report to the listener
                listener->OnPhotonReport(move(report));
            } else {
                LOGERROR("No listener to send frame to");
            }

            // if the usb callback hasn't been called by now we need to cancel it
            if(activeTransfer)
            {
                device.CancelTransfer(activeTransfer);
            }
            frame++;

        }/*lock scope*/

    }

    void UsbTagger::DataPusher::ReadDataAsync(std::unique_ptr<DataBlock> data)
    {
        activeTransfer = nullptr;

        {/*lock scope*/
            unique_lock<mutex> lock(processingQueueMutex);
            // move the data onto the processing queue
            processingQueue.push(move(data));
        }/*lock scope*/

        // trigger the converter to process it
        dataReadyCv.notify_one();

        if(!shutdown && keepReading)
        {
            // start off the next transfer
            activeTransfer = device.StartReadingBulk(bulkReadRequest, GetBuffer(), &DataPusher::ReadDataAsync, this);
        }
    }

    void UsbTagger::DataPusher::ConvertData()
    {
        using NoxReport = fs::DataFile::NoxReport;
        NoxReport devReport;
        DataBlockPtr data;

        while(!shutdown)
        {
            {/*lock scope*/
                unique_lock<mutex> lock(processingQueueMutex);
                dataReadyCv.wait(lock, [&](){
                    return !processingQueue.empty() || shutdown;
                });

                if(!shutdown)
                {
                    data = move(processingQueue.front());
                    processingQueue.pop();
                }
            }/*lock scope*/

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
            } // if data

            // trigger anything waiting for us to finish
            dataReadyCv.notify_one();
        }// while !stopProcessing

    } // ConvertData

    UsbTagger::DataPusher::DataBlockPtr UsbTagger::DataPusher::GetBuffer()
    {
        using namespace std;
        DataBlockPtr result;

        unique_lock<mutex> lock(unusedQueueMutex);
        if(unusedBuffers.empty())
        {
            // there are no free buffers, make another
            result = make_unique<DataBlock>();
        } else {
            result = move(unusedBuffers.front());
            unusedBuffers.pop();
        }
        // prepare the buffer for receiving data
        result->resize(maxBulkRead);

        return result;
    }

    void UsbTagger::DataPusher::ReturnBuffer(UsbTagger::DataPusher::DataBlockPtr buffer)
    {
        using namespace std;
        unique_lock<mutex> lock(unusedQueueMutex);
        unusedBuffers.push(move(buffer));
    }

    // ******* UsbTagger methods **************

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
} // namespace cqp
