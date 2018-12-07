#pragma once
#include <string>
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/DetectionReport.h"
#include <fstream>
#include "Algorithms/Logging/Logger.h"
#include <sstream>

namespace cqp {
    namespace fs {

        /**
         * @brief The DataFile class
         * Read and write various data formats
         */
        class ALGORITHMS_EXPORT DataFile
        {
        public:
            DataFile() = default;
            /**
             * @brief ReadPackedQubits
             * Read a list of qubits from a packed binary file.
             * @details
             * 2 bits per qubit, 4 qubits per byte.
             * @param inFileName
             * @param output
             * @param maxValues Maximum number of values to get. 0 = no limit
             * @return True on success
             */
            static bool ReadPackedQubits(const std::string& inFileName, QubitList& output, uint64_t maxValues = 0);

            /**
             * @brief WriteQubits
             * Write a list of qubits into a packed binary file.
             * @param source
             * @param outFileName
             * @return true on success
             */
            static bool WriteQubits(const QubitList& source, const std::string& outFileName);

            /// Specifies that channel 0 == BB84::Zero, channel 1 == BB84::One, etc
            static const std::vector<Qubit> DefautlCahnnelMappings; // = { 0, 1, 2, 3 };

            /**
             * @brief ReadNOXDetections
             * Read the proprietary format for the NOX time tagger
             * @details
             * 8 bytes per record, prefixed by ether a $  (0x24) or % (0x25)
             * | Bit number | Num Bits  | Description           |
             * | 0 - 7      | 8         | Type of record, % (0x25) = config data, $ (0x24) = detection |
             * | 8 - 43     | 36        | Course counter bits   |
             * | 44 - 47    | 4         | blank                 |
             * | 48 - 51    | 4         | channel               |
             * | 52 - 63    | 12        | Fine count            |
             * @param inFileName
             * @param output
             * @param waitForConfig If true, drop records before the first config record
             * @param maxCourseTime If none-zero, stop reading records when the course time reaches this value.
             * @return true on success
             */
            static bool ReadNOXDetections(const std::string& inFileName, DetectionReports& output,
                                          const std::vector<Qubit>& channelMappings = DefautlCahnnelMappings, bool waitForConfig = true, uint64_t maxCourseTime = 0);

            /**
             * @brief ReadDetectionReportList
             * @details
             * 64bit integer number of picoseconds in network byte order
             * 1 byte Qubit value
             * @param inFileName Filename to read from
             * @param output destination for report
             * @return true on success
             */
            static bool ReadDetectionReportList(const std::string& inFileName, DetectionReports& output);

            /**
             * @brief WriteDetectionReportList
             * @details
             * 64bit integer number of picoseconds in network byte order
             * @param source Data to write
             * @param outFileName Filename for output
             * @return true on success
             */
            static bool WriteDetectionReportList(const DetectionReports& source, const std::string& outFileName);

            /**
             * Defines the messages sent by the NOX box
             */
            struct NoxReport
            {
            public: // types
                /// The possible message types
                enum class MessageType : uint8_t {
                    Invalid = 0,
                    Config = 0x25,
                    Detection = 0x24
                };
                /// The ratio of clock ticks to seconds
                using CourseTime = std::chrono::duration<uint64_t, std::ratio<1, 130000000>>;
                /// The number of ticks per clock cycle
                static constexpr const auto fineRatio = CourseTime::period::den * 4096;
                /// The ratio of fine taps to seconds
                using FineTime = std::chrono::duration<uint64_t, std::ratio<1, fineRatio>>;
                /// The structure of the detection message
                struct Detection {
                    /// course time value
                    uint64_t coarse {0};
                    /// fine time value
                    uint16_t fine {0};
                    /// detection channel
                    uint8_t channel {0};
                };

                using Buffer = std::array<uint8_t, 8>;

            public: // members
                /// Possible variations of the message
                Detection detection;
                /// The message type
                MessageType messageType = MessageType::Invalid;

            public: // methods
                /**
                 * @brief GetTime
                 * @return Time in picoseconds
                 */
                PicoSeconds GetTime() const;

            };

            template<class Iter>
            static bool WriteCSV(const std::string& filename, const Iter& begin, const Iter& end,
                                 const std::string& seperator = ", ")
            {
                bool result = false;
                try {
                    std::ofstream outFile(filename, std::ios::out | std::ios::binary);
                    if (outFile)
                    {
                        for(auto it = begin; it != end; it++)
                        {
                            std::stringstream value;
                            if(it != begin)
                            {
                                value << seperator;
                            }
                            value << *it;
                            outFile.write(value.str().data(), value.str().size());
                        }
                        outFile.close();
                        result = true;
                    }

                } catch (const std::exception& e) {
                    LOGERROR(e.what());
                }

                return result;
            }
        };

    } // namespace fs
} // namespace cqp


