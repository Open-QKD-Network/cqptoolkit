/*!
* @file
* @brief IDQKeyExtraction
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 1/5/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Algorithms/Logging/ConsoleLogger.h"
#include "Algorithms/Util/FileIO.h"
#include "Algorithms/Util/DataFile.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Alignment/Filter.h"
#include "Algorithms/Alignment/Gating.h"
#include "QKDPostProc.h"

#include <thread>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <future>

using namespace cqp;

struct Names
{
    static CONSTSTRING noxData= "noxdata";
    static CONSTSTRING packedQubits= "packed";
    static CONSTSTRING slotWidth = "slotWidth";
    static CONSTSTRING pulseWidth = "pulseWidth";
};

QKDPostProc::QKDPostProc()
{
    using std::placeholders::_1;

    cqp::ConsoleLogger::Enable();
    DefaultLogger().SetOutputLevel(LogLevel::Debug);

    definedArguments.AddOption("help", "h", "display help information on command line arguments")
    .Callback(std::bind(&QKDPostProc::DisplayHelp, this, _1));

    definedArguments.AddOption("", "q", "Decrease output")
    .Callback(std::bind(&QKDPostProc::HandleQuiet, this, _1));

    definedArguments.AddOption("", "v", "Increase output")
    .Callback(std::bind(&QKDPostProc::HandleVerbose, this, _1));

    definedArguments.AddOption(Names::noxData, "n", "Read NoxBox Detections from file")
            .Bind();
    definedArguments.AddOption(Names::packedQubits, "p", "Read transmissions from packed Qubits file")
            .Bind();
    definedArguments.AddOption(Names::slotWidth, "", "Slot width of transmissions (Picoseconds)")
            .Bind();
    definedArguments.AddOption(Names::pulseWidth, "", "Pulse width of photon (Picoseconds)")
            .Bind();
}

void QKDPostProc::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Processes QKD data using different parameters to produce key.");
    definedArguments.StopOptionsProcessing();
    stopExecution = true;
}

struct RunData {
    PicoSecondOffset offset;
    uint64_t highestValue = 0;
    size_t highestIndex = 0;
};

int QKDPostProc::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {

        std::string detectionsFile = "BobDetections.bin";
        DetectionReportList detections;
        definedArguments.GetProp(Names::noxData, detectionsFile);

        if(!fs::DataFile::ReadNOXDetections(detectionsFile, detections))
        {
            LOGERROR("Failed to open file: " + detectionsFile);
        }else {
            // to isolate the transmission
            align::Filter filter;
            // to find the qubits
            align::Gating gating(rng);
            // bobs qubits
            QubitList RecverResults;
            DetectionReportList::const_iterator start;
            DetectionReportList::const_iterator end;

            filter.Isolate(detections, start, end);
            LOGINFO("Detections: " + std::to_string(detections.size()) +
                    " Start: " + std::to_string(distance(detections.cbegin(), start)) +
                    " End: " + std::to_string(distance(detections.cbegin(), end)));

            align::Gating::ValidSlots validSlots;
            gating.ExtractQubits(start, end, validSlots, RecverResults);

            LOGINFO("Found " + std::to_string(RecverResults.size()) + " Qubits over " + to_string(validSlots.size()) + " slots.");
            fs::DataFile::WriteQubits(RecverResults, "BobQubits.bin");

            ////////////// Load Alice data to compare with Bobs
            std::string packedFile = "AliceRandom.bin";
            definedArguments.GetProp(Names::packedQubits, packedFile);

            QubitList aliceQubits;
            LOGDEBUG("Loading Alice data file");
            if(!fs::DataFile::ReadPackedQubits(packedFile, aliceQubits))
            {
                LOGERROR("Failed to open transmisser file: " + packedFile);
            } else {
                if(gating.FilterDetections(validSlots.cbegin(), validSlots.cend(), aliceQubits) && RecverResults.size() == aliceQubits.size())
                {
                    size_t validCount = 0;
                    LOGINFO("Comparing " + std::to_string(aliceQubits.size()) + " qubits...");
                    for(auto index = 0u; index < RecverResults.size(); index++)
                    {
                        if(RecverResults[index] == aliceQubits[index])
                        {
                            validCount++;
                        }
                    }
                    const double errorRate = 1.0 - static_cast<double>(validCount) / RecverResults.size();
                    LOGINFO("Valid qubits: " + std::to_string(validCount) + " out of " + std::to_string(aliceQubits.size()) +
                            ". Error rate: " + std::to_string(errorRate * 100) + "%");
                } else {
                    LOGERROR("Failed to filter alice detections");
                }
            }
        }


    }

    return exitCode;
}

CQP_MAIN(QKDPostProc)
