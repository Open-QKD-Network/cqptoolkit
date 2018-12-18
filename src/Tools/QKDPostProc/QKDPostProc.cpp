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
#include "Algorithms/Random/RandomNumber.h"
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
    static CONSTSTRING driftMax = "drift-max";
    static CONSTSTRING driftStep = "drift-step";
    static CONSTSTRING bins = "bins";
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
    definedArguments.AddOption(Names::driftMax, "", "Find Drift: Maximum offset to search (Pico Seconds)")
            .Bind();
    definedArguments.AddOption(Names::driftStep, "", "Find Drift: offset between each offset to try(Picoseconds)")
            .Bind();
    definedArguments.AddOption(Names::bins, "", "Number of bins to split the detections into")
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
        std::shared_ptr<RandomNumber> rng{new RandomNumber()};
        if(!fs::DataFile::ReadNOXDetections(detectionsFile, detections))
        {
            LOGERROR("Failed to open file: " + detectionsFile);
        }else {
            align::Filter filter;
            align::Gating gating(rng);
            DetectionReportList::const_iterator start;
            DetectionReportList::const_iterator end;
            filter.Isolate(detections, start, end);
            LOGINFO("Detections: " + std::to_string(detections.size()) +
                    " Start: " + std::to_string(distance(detections.cbegin(), start)) +
                    " End: " + std::to_string(distance(detections.cbegin(), end)));

            QubitList results;
            align::Gating::ValidSlots validSlots;
            gating.ExtractQubits(start, end, validSlots, results);

            LOGINFO("Found " + std::to_string(results.size()) + " Qubits over " + to_string(validSlots.size()) + " slots.");
            fs::DataFile::WriteQubits(results, "BobQubits.bin");
        }

    }

    return exitCode;
}

CQP_MAIN(QKDPostProc)
