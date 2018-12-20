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
    static CONSTSTRING slotsPerFrame = "slots-per-frame";
    static CONSTSTRING slotWidth = "slot-width";
    static CONSTSTRING pulseWidth = "pulse-width";
    static CONSTSTRING samplesPerFrame = "samples";
    static CONSTSTRING acceptanceRatio = "acceptance";
    static CONSTSTRING filterSigma = "filter-sigma";
    static CONSTSTRING filterWidth = "filter-width";
    static CONSTSTRING filterCutoff = "filter-cutoff";
    static CONSTSTRING filterStride = "filter-stride";
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
    definedArguments.AddOption(Names::slotsPerFrame, "", "Length of transmission in number of slots")
            .Bind();
    definedArguments.AddOption(Names::slotWidth, "", "Slot width of transmissions (Picoseconds)")
            .Bind();
    definedArguments.AddOption(Names::pulseWidth, "", "Pulse width of photon (Picoseconds)")
            .Bind();
    definedArguments.AddOption(Names::samplesPerFrame, "", "Split the data into this many samples to calculate drift")
            .Bind();
    definedArguments.AddOption(Names::acceptanceRatio, "", "Value between 0 and 1 for the gating filter")
            .Bind();
    definedArguments.AddOption(Names::filterSigma, "", "Sigma value for the gaussian filter")
            .Bind();
    definedArguments.AddOption(Names::filterWidth, "", "Integer width in gaussian filter")
            .Bind();
    definedArguments.AddOption(Names::filterCutoff, "", "Percentage for filter acceptance")
            .Bind();
    definedArguments.AddOption(Names::filterStride, "", "Data items to skip when filtering")
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

        auto slotsPerFrame = align::Gating::DefaultSlotsPerFrame;
        definedArguments.GetProp(Names::slotsPerFrame, slotsPerFrame);

        auto slotWidth = align::Gating::DefaultSlotWidth;
        definedArguments.GetProp(Names::slotWidth, slotWidth);

        auto pulseWidth = align::Gating::DefaultPulseWidth;
        definedArguments.GetProp(Names::pulseWidth, pulseWidth);

        auto samplesPerFrame = align::Gating::DefaultSamplesPerFrame;
        definedArguments.GetProp(Names::samplesPerFrame, samplesPerFrame);

        auto acceptanceRatio = align::Gating::DefaultAcceptanceRatio;
        definedArguments.GetProp(Names::acceptanceRatio, acceptanceRatio);

        auto filterSigma = align::Filter::DefaultSigma;
        definedArguments.GetProp(Names::filterSigma, filterSigma);

        auto filterWidth = align::Filter::DefaultFilterWidth;
        definedArguments.GetProp(Names::filterWidth, filterWidth);

        auto filterCutoff = align::Filter::DefaultCutoff;
        definedArguments.GetProp(Names::filterCutoff, filterCutoff);

        auto filterStride = align::Filter::DefaultStride;
        definedArguments.GetProp(Names::filterStride, filterStride);

        if(!fs::DataFile::ReadNOXDetections(detectionsFile, detections))
        {
            LOGERROR("Failed to open file: " + detectionsFile);
        }else {
            // to isolate the transmission
            align::Filter filter(filterSigma, filterWidth, filterCutoff, filterStride);
            // to find the qubits
            align::Gating gating(rng, slotsPerFrame, slotWidth, pulseWidth, samplesPerFrame, acceptanceRatio);
            // bobs qubits
            QubitList receiverResults;
            DetectionReportList::const_iterator start;
            DetectionReportList::const_iterator end;

            filter.Isolate(detections, start, end);
            const auto window = (end - 1)->time - start->time;

            LOGINFO("Detections: " + std::to_string(detections.size()) + "\n" +
                    " Start: " + std::to_string(distance(detections.cbegin(), start)) + "\n" +
                    " End: " + std::to_string(distance(detections.cbegin(), end)) + "\n" +
                    " Duration: " + std::to_string(chrono::duration_cast<SecondsDouble>(window).count()) + "s");

            align::Gating::ValidSlots validSlots;
            const auto startTime = std::chrono::high_resolution_clock::now();
            //gating.SetDrift(PicoSeconds(34795));
            gating.ExtractQubits(start, end, validSlots, receiverResults);
            const auto extractTime = std::chrono::high_resolution_clock::now() - startTime;

            LOGINFO("Found " + to_string(receiverResults.size()) + " Qubits over " + to_string(validSlots.size()) + " slots. In " +
                    to_string(chrono::duration_cast<chrono::milliseconds>(extractTime).count()) + "ms");
            fs::DataFile::WriteQubits(receiverResults, "BobQubits.bin");

            ////////////// Load Alice data to compare with Bobs
            std::string packedFile = "AliceRandom.bin";
            definedArguments.GetProp(Names::packedQubits, packedFile);

            QubitList aliceQubits;
            LOGDEBUG("Loading Alice data file");
            if(!fs::DataFile::ReadPackedQubits(packedFile, aliceQubits))
            {
                LOGERROR("Failed to open transmisser file: " + packedFile);
            } else {

                const auto numQubitsSent = aliceQubits.size();
                if(gating.FilterDetections(validSlots.cbegin(), validSlots.cend(), aliceQubits) && receiverResults.size() == aliceQubits.size())
                {
                    size_t validCount = 0;
                    LOGINFO("Comparing " + std::to_string(aliceQubits.size()) + " qubits...");
                    for(auto index = 0u; index < receiverResults.size(); index++)
                    {
                        if(receiverResults[index] == aliceQubits[index])
                        {
                            validCount++;
                        }
                    }
                    const double errorRate = 1.0 - static_cast<double>(validCount) / receiverResults.size();
                    const double detectionRate = static_cast<double>(validCount) / numQubitsSent;

                    LOGINFO("Valid qubits: " + std::to_string(validCount) + " out of " + std::to_string(aliceQubits.size()) +
                            ".\n Error rate: " + std::to_string(errorRate * 100) + "%");

                    cout << "Step, Slot Width, Pulse Width, Samples Per Frame, "
                         << "Acceptance Ratio, Qubits sent, Qubits detected, Valid Qubits, "
                         << "Detection Rate, Error rate, Time (us)" << endl;
                    cout << "Gating, " << slotWidth.count() << ", " << pulseWidth.count() << ", " << samplesPerFrame << ", "
                         << acceptanceRatio << ", " << numQubitsSent << ", " << aliceQubits.size() << ", " << validCount << ", "
                         << detectionRate << ", " << errorRate << ", " << chrono::duration_cast<chrono::microseconds>(extractTime).count() << endl;
                } else {
                    LOGERROR("Failed to filter alice detections");
                }
            }
        }


    }

    return exitCode;
}

CQP_MAIN(QKDPostProc)
