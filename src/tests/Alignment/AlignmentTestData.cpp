/*!
* @file
* @brief AlignmentTestData
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 21/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "AlignmentTestData.h"
#include <fstream>
#include "CQPToolkit/Util/Util.h"
#include "CQPToolkit/Util/Logger.h"
#include <chrono>

namespace cqp
{
    namespace test
    {

        void AlignmentTestData::LoadGated(const std::string& txFile)
        {
            std::ifstream inFile(txFile, std::ios::in | std::ios::binary);
            if (inFile)
            {
                uint64_t index = 0;
                // qubits packed 4/byte
                inFile.seekg(0, std::ios::end);
                emissions.resize(static_cast<size_t>(inFile.tellg() * 4));
                inFile.seekg(0, std::ios::beg);

                while(!inFile.eof())// && (index * emissionDelay) <= emissionPeriod)
                {
                    char packedQubits;
                    inFile.read(&packedQubits, sizeof (packedQubits));

                    emissions[index++] = (packedQubits & 0b00000011);
                    emissions[index++] = ((packedQubits & 0b00001100) >> 2);
                    emissions[index++] = ((packedQubits & 0b00110000) >> 4);
                    emissions[index++] = ((packedQubits & 0b11000000) >> 6);
                }
                inFile.close();
            }
        }

        void AlignmentTestData::LoadBobDetections(const std::string& txFile)
        {
            using namespace std;
            // csv file with qubit and double

            std::ifstream inFile(txFile, std::ios::in | std::ios::binary);
            if (inFile)
            {
                std::string line;
                vector<string> elements;

                while(!inFile.eof())
                {
                    getline(inFile, line);

                    elements.clear();
                    SplitString(line, elements, ",");
                    if(elements.size() == 2)
                    {
                        try
                        {
                            using namespace std::chrono;
                            DetectionReport report;

                            report.value = static_cast<Qubit>(std::stoul(elements[0]) - 1);
                            const double seconds = std::stod(elements[1]);

                            report.time = duration_cast<PicoSeconds>(duration<double>(seconds));

                            detections.push_back(report);
                        }
                        catch (const exception& e)
                        {
                            LOGERROR(e.what());
                        }
                    }
                }
                inFile.close();
            }
        }

    } // namespace test
} // namespace cqp
