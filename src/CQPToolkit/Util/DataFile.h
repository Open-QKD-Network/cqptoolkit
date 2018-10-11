#pragma once
#include <string>
#include "CQPToolkit/Datatypes/Qubits.h"
#include "CQPToolkit/Datatypes/DetectionReport.h"


namespace cqp {
    namespace fs {

        class CQPTOOLKIT_EXPORT DataFile
        {
        public:
            DataFile();
            /**
             * @brief ReadPackedQubits
             * Read a list of qubits from a packed binary file.
             * @details
             * 2 bits per qubit, 4 qubits per byte.
             * @param inFile
             * @param output
             * @return True on success
             */
            static bool ReadPackedQubits(const std::string& inFileName, QubitList& output);

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
             * @param inFile
             * @param output
             * @return true on success
             */
            static bool ReadNOXDetections(const std::string& inFileName, DetectionReportList& output,
                                          const std::vector<Qubit>& channelMappings = DefautlCahnnelMappings, bool waitForConfig = true);

            /**
             * @brief ReadDetectionReportList
             * @details
             * 64bit integer number of picoseconds in network byte order
             * 1 byte Qubit value
             * @param inFile
             * @param output
             * @return true on success
             */
            static bool ReadDetectionReportList(const std::string& inFileName, DetectionReportList& output);

            /**
             * @brief WriteDetectionReportList
             * @details
             * 64bit integer number of picoseconds in network byte order
             * @param source
             * @param outFile
             * @return true on success
             */
            static bool WriteDetectionReportList(const DetectionReportList& source, const std::string& outFileName);

            /// Defines the messages sent by the NOX box
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
                using CourseTime = std::chrono::duration<uint64_t, std::ratio<1, 225000000>>;
                /// The number of ticks per clock cycle
                static constexpr const auto fineRatio = CourseTime::period::den * 4096;
                /// The ratio of fine taps to seconds
                using FineTime = std::chrono::duration<uint64_t, std::ratio<1, fineRatio>>;
                /// The structure of the detection message
                struct Detection {
                    uint64_t coarse {0};
                    uint16_t fine {0};
                    uint8_t channel {0};
                };

                using Buffer = std::array<uint8_t, 8>;

            public: // members
                /// Possible variations of the message
                Detection detection;
                MessageType messageType = MessageType::Invalid;

            public: // methods
                PicoSeconds GetTime() const;

            };
        };

    } // namespace fs
} // namespace cqp


