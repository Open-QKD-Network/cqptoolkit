#include "DataFile.h"
#include <fstream>
#include "CQPToolkit/Util/Logger.h"
#include <chrono>
#include <cmath>

namespace cqp
{
    namespace fs
    {

        DataFile::DataFile()
        {

        }

        bool DataFile::ReadPackedQubits(const std::string& inFileName, QubitList& output)
        {
            bool result = true;
            uint64_t index = 0;
            std::ifstream inFile(inFileName, std::ios::in | std::ios::binary);
            if (inFile)
            {
                // qubits packed 4/byte
                inFile.seekg(0, std::ios::end);
                output.resize(static_cast<size_t>(inFile.tellg() * 4));
                inFile.seekg(0, std::ios::beg);

                while(!inFile.eof())
                {
                    char packedQubits;
                    inFile.read(&packedQubits, sizeof (packedQubits));

                    output[index++] = (packedQubits & 0b00000011);
                    output[index++] = ((packedQubits & 0b00001100) >> 2);
                    output[index++] = ((packedQubits & 0b00110000) >> 4);
                    output[index++] = ((packedQubits & 0b11000000) >> 6);
                }
                inFile.close();
                result = true;
            }
            else
            {
                LOGERROR("Failed to open " + inFileName);
            }

            return result;
        }

        bool DataFile::WriteQubits(const QubitList& source, const std::string& outFileName)
        {
            bool result = false;
            std::ofstream outFile(outFileName, std::ios::out | std::ios::binary);
            if (outFile)
            {
                for(uint64_t index = 0; index < source.size(); index += 4)
                {
                    uint8_t buffer {0};

                    buffer = static_cast<uint8_t>(source[index] |
                                                  (source[index+1] << 2) |
                                                  (source[index+2] << 4) |
                                                  (source[index+3] << 6));
                    outFile.write(reinterpret_cast<char*>(&buffer), sizeof (buffer));
                }

                const uint8_t remainder = source.size() % 4;
                if(remainder != 0)
                {
                    LOGWARN("file will be padded with trailing zeros to the nearest byte");
                    uint8_t shiftOffset {0};
                    uint8_t buffer {0};

                    for(uint64_t index = source.size() - remainder - 1; index < source.size(); index++)
                    {
                        buffer |= source[index] << shiftOffset;

                        shiftOffset += 2;
                    }
                    // store the remainder
                    outFile.write(reinterpret_cast<char*>(&buffer), sizeof (buffer));
                }
                outFile.close();
                result = true;

            }
            else
            {
                LOGERROR("Failed to open " + outFileName);
            }

            return result;
        }

        const std::vector<Qubit> DataFile::DefautlCahnnelMappings = { 0, 1, 2, 3 };

        bool DataFile::ReadNOXDetections(const std::string& inFileName, DetectionReportList& output,
                                         const  std::vector<Qubit>& channelMappings, bool waitForConfig)
        {

            bool result = true;
            uint64_t droppedDetections = 0;
            std::ifstream inFile(inFileName, std::ios::in | std::ios::binary);
            if (inFile)
            {
                // qubits packed 4/byte
                inFile.seekg(0, std::ios::end);
                const auto fileSize = inFile.tellg() * 4;
                const size_t numRecords = static_cast<size_t>(fileSize / 8);
                if(fileSize % 8 != 0)
                {
                    LOGERROR("File is invalid.");
                }
                else
                {
                    inFile.seekg(0, std::ios::beg);
                    output.reserve(numRecords);
                    bool gotConfig = !waitForConfig;

                    do
                    {
                        NoxReport noxReport;
                        NoxReport::Buffer buffer {0};

                        inFile.read(reinterpret_cast<char*>(&buffer), sizeof (buffer));
                        noxReport.messageType = static_cast<NoxReport::MessageType>(buffer[0]);

                        if(!inFile.eof())
                        {
                            if(gotConfig && noxReport.messageType == NoxReport::MessageType::Detection)
                            {
                                noxReport.detection.coarse = (
                                                                 static_cast<uint64_t>(buffer[1]) <<28 |
                                                                 static_cast<uint64_t>(buffer[2]) <<20 |
                                                                 static_cast<uint64_t>(buffer[3]) <<12 |
                                                                 static_cast<uint64_t>(buffer[4]) <<4 |
                                                                 static_cast<uint64_t>(buffer[5]) >>4
                                                             );

                                noxReport.detection.fine = static_cast<uint16_t>((buffer[6] & 0x0F) << 8) | buffer[7];

                                noxReport.detection.channel = (buffer[6] >> 4) -1;

                                if(noxReport.detection.channel < channelMappings.size())
                                {
                                    DetectionReport report;
                                    report.value = channelMappings[noxReport.detection.channel];
                                    report.time = noxReport.GetTime();
                                    output.push_back(report);
                                }
                                else
                                {
                                    LOGWARN("Channel " + std::to_string(noxReport.detection.channel) + " not mapped.");
                                    droppedDetections++;
                                }

                            }
                            else if(noxReport.messageType == NoxReport::MessageType::Config)
                            {
                                gotConfig = true;

                            }
                            result = true;
                        }
                    }
                    while(!inFile.eof());
                    inFile.close();
                    LOGINFO("Dropped " + std::to_string(droppedDetections) + " detections");
                }
            }
            else
            {
                LOGERROR("Failed to open " + inFileName);
            }

            return result;
        }

        bool DataFile::ReadDetectionReportList(const std::string& inFileName, DetectionReportList& output)
        {
            bool result = false;
            std::ifstream inFile(inFileName, std::ios::in | std::ios::binary);
            if (inFile)
            {
                inFile.seekg(0, std::ios::end);
                output.reserve(static_cast<uint64_t>(inFile.tellg()) / (sizeof(uint64_t) + sizeof(Qubit)));
                inFile.seekg(0, std::ios::beg);

                do
                {
                    DetectionReport report;
                    uint64_t time {0};
                    inFile.read(reinterpret_cast<char*>(&time), sizeof (time));
                    inFile.read(reinterpret_cast<char*>(&report.value), sizeof (report.value));
                    if(!inFile.eof())
                    {
                        report.time = PicoSeconds(be64toh(time));

                        output.push_back(report);
                    }
                }
                while(!inFile.eof());

                inFile.close();
                result = true;
            }
            else
            {
                LOGERROR("Failed to open " + inFileName);
            }

            return result;
        }

        bool DataFile::WriteDetectionReportList(const DetectionReportList& source, const std::string& outFileName)
        {
            bool result = false;
            std::ofstream outFile(outFileName, std::ios::out | std::ios::binary);
            if (outFile)
            {
                for(const auto& report : source)
                {
                    const uint64_t time = htobe64(report.time.count());
                    outFile.write(reinterpret_cast<const char*>(&time), sizeof(time));
                    outFile.write(reinterpret_cast<const char*>(&report.value), sizeof (report.value));
                }
                outFile.close();
                result = true;

            }
            else
            {
                LOGERROR("Failed to open " + outFileName);
            }

            return result;
        }

        PicoSeconds DataFile::NoxReport::GetTime() const
        {
            using namespace std::chrono;
            PicoSeconds result{0};
            if(messageType == MessageType::Detection)
            {
                // convert to host byte order and store in a type which
                // understands the scale of the value.
                const CourseTime courseTime{detection.coarse};
                const FineTime fineTime{detection.fine};

                // convert to Picoseconds
                result = duration_cast<PicoSeconds>(courseTime + fineTime);
            }
            return result;
        }

    } // namespace fs
} // namespace cqp
