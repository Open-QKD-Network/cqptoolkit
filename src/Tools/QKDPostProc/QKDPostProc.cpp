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
#include "Algorithms/Alignment/Drift.h"
#include "QKDPostProc.h"
#include "Algorithms/Alignment/Offsetting.h"

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
    static CONSTSTRING slotWidth = "slot-width";
    static CONSTSTRING pulseWidth = "pulse-width";
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
    definedArguments.AddOption(Names::slotWidth, "", "Slot width of transmissions (Picoseconds)")
            .Bind();
    definedArguments.AddOption(Names::pulseWidth, "", "Pulse width of photon (Picoseconds)")
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

        PicoSeconds slotWidth = std::chrono::nanoseconds(100);
        definedArguments.GetProp(Names::slotWidth, slotWidth);

        PicoSeconds pulseWidth = std::chrono::nanoseconds(1);
        definedArguments.GetProp(Names::pulseWidth, pulseWidth);

        auto acceptanceRatio = align::Gating::DefaultAcceptanceRatio;
        definedArguments.GetProp(Names::acceptanceRatio, acceptanceRatio);

        auto filterSigma = align::Filter::DefaultSigma;
        definedArguments.GetProp(Names::filterSigma, filterSigma);

        auto filterWidth = align::Filter::DefaultFilterWidth;
        definedArguments.GetProp(Names::filterWidth, filterWidth);

        auto filterStride = align::Filter::DefaultStride;
        definedArguments.GetProp(Names::filterStride, filterStride);

        double highestMatch = 0.0;
        ssize_t highestOffset = 0;
        std::vector<Qubit> highestChannelMappings;

        //const std::vector<Qubit> channelMappings = { 0, 1, 2, 3 }; // 75.569566%
        //const std::vector<Qubit> channelMappings = { 1, 0, 2, 3 }; // 24.4%
        //const std::vector<Qubit> channelMappings = { 2, 0, 1, 3 }; // 31.43%
        //const std::vector<Qubit> channelMappings = { 0, 2, 1, 3 }; // 58.63%
        //const std::vector<Qubit> channelMappings = { 1, 2, 0, 3 }; // 41.36%
        //const std::vector<Qubit> channelMappings = { 2, 1, 0, 3 }; // 68.53%
        //const std::vector<Qubit> channelMappings = { 2, 1, 3, 0 }; // 68.70%
        //const std::vector<Qubit> channelMappings = { 1, 2, 3, 0 }; // 41.53%
        //const std::vector<Qubit> channelMappings = { 3, 2, 1, 0 }; // 50.16%
        //const std::vector<Qubit> channelMappings = { 2, 3, 1, 0 }; // 50.15%
        //const std::vector<Qubit> channelMappings = { 1, 3, 2, 0 }; // 41.52%
        //const std::vector<Qubit> channelMappings = { 3, 1, 2, 0 }; // 68.70%
        //const std::vector<Qubit> channelMappings = { 3, 0, 2, 1 }; // 31.29%
        //const std::vector<Qubit> channelMappings = { 0, 3, 2, 1 }; // 58.47%
        //const std::vector<Qubit> channelMappings = { 2, 3, 0, 1 }; // 49.84%
        //const std::vector<Qubit> channelMappings = { 3, 2, 0, 1 }; // 49.84%
        //const std::vector<Qubit> channelMappings = { 0, 2, 3, 1 }; // 58.48%
        //const std::vector<Qubit> channelMappings = { 2, 0, 3, 1 }; // 31.29%
        //const std::vector<Qubit> channelMappings = { 1, 0, 3, 2 }; // 24.42%
        //const std::vector<Qubit> channelMappings = { 0, 1, 3, 2 }; // 75.559871%
        //const std::vector<Qubit> channelMappings = { 3, 1, 0, 2 }; // 68.57%
        //const std::vector<Qubit> channelMappings = { 1, 3, 0, 2 }; // 41.37%
        //const std::vector<Qubit> channelMappings = { 0, 3, 1, 2 }; // 58.61%
        //const std::vector<Qubit> channelMappings = { 3, 0, 1, 2 }; // 31.43%
        auto mapping = 0u;
        for(std::vector<Qubit> channelMappings : std::vector<std::vector<Qubit>>({
        /*{ 0, 1, 2, 3 },
        { 1, 0, 2, 3 },
        { 2, 0, 1, 3 },
        { 0, 2, 1, 3 },
        { 1, 2, 0, 3 },
        { 2, 1, 0, 3 },
        { 2, 1, 3, 0 },
        { 1, 2, 3, 0 },
        { 3, 2, 1, 0 },
        { 2, 3, 1, 0 },
        { 1, 3, 2, 0 },
        { 3, 1, 2, 0 },
        { 3, 0, 2, 1 },
        { 0, 3, 2, 1 },
        { 2, 3, 0, 1 },
        { 3, 2, 0, 1 },
        { 0, 2, 3, 1 },
        { 2, 0, 3, 1 },
        { 1, 0, 3, 2 },
        { 0, 1, 3, 2 },*/
        { 3, 1, 0, 2 }//,
        /*{ 1, 3, 0, 2 },
        { 0, 3, 1, 2 },
        { 3, 0, 1, 2 }*/
    }))
        {
            detections.clear();
            LOGDEBUG("Mapping: " + to_string(mapping++));
            if(!fs::DataFile::ReadNOXDetections(detectionsFile, detections, channelMappings))
            {
                LOGERROR("Failed to open file: " + detectionsFile);
            }else {
                // to isolate the transmission
                align::Filter filter(filterSigma, filterWidth, align::Filter::DefaultCourseTheshold, align::Filter::DefaultFineTheshold,
                                     filterStride);
                // to find the qubits
                align::Gating gating(rng, slotWidth, pulseWidth, acceptanceRatio);
                align::Drift drift(slotWidth, pulseWidth);
                // bobs qubits
                QubitList receiverResults;
                DetectionReportList::const_iterator start;
                DetectionReportList::const_iterator end;

                filter.Isolate(detections, start, end);
                const auto window = (end - 1)->time - start->time;

                /*{
                    std::ofstream datafile = ofstream("filtered.csv");
                    for(auto filteredIt = start; filteredIt != end; filteredIt++)
                    {
                        datafile << to_string(filteredIt->time.count()) << ", " << to_string(filteredIt->value) << endl;
                    }
                    datafile.close();
                }*/

                //start = detections.cbegin() + 113141;
                //end = detections.cbegin() + 706150;
                // start should be @ 2.0377699934326174
                LOGINFO("Detections: " + std::to_string(distance(start, end)) + "\n" +
                        " Start: " + std::to_string(distance(detections.cbegin(), start)) + " @ " + to_string(start->time.count()) + "pS\n" +
                        " End: " + std::to_string(distance(detections.cbegin(), end)) + " @ " + to_string(end->time.count()) + "pS\n" +
                        " Duration: " + std::to_string(chrono::duration_cast<SecondsDouble>(window).count()) + "s");

                align::Gating::ValidSlots validSlots;
                const auto startTime = std::chrono::high_resolution_clock::now();
                //gating.SetDrift(PicoSecondOffset(34795));
                //gating.SetDrift(PicoSecondOffset(34610));
                //gating.SetDrift(PicoSecondOffset(34470));
                //gating.SetDrift(PicoSecondOffset(33870));
                gating.SetDrift(drift.Calculate(start, end));
                gating.ExtractQubits(start, end, validSlots, receiverResults);
                const auto extractTime = std::chrono::high_resolution_clock::now() - startTime;

                LOGINFO("Found " + to_string(receiverResults.size()) + " Qubits, last slot ID: " + to_string(*validSlots.rbegin()) + ". Took " +
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

                    align::Offsetting offsetting(1000);

                    for(auto offset = 0; offset < 8000; offset++)
                    {
                        //LOGDEBUG("================== Offset: " + to_string(offset) + " =============");

                        /*{
                            std::ofstream datafile = ofstream("received.csv");
                            for(auto index = 0u; index < receiverResults.size(); index++)
                            {
                                datafile << to_string(validSlots[index]) << ", " << to_string(receiverResults[index]) << endl;
                            }
                            datafile.close();
                        }*/

/*
                        double confidence = 0.5;
                        size_t basesMatched = 0;
                        size_t validCount = 0;
                        const auto step = receiverResults.size() / 5000;
                        const auto numToCheck = receiverResults.size();

                        for(uint64_t index = 0; index < numToCheck; index = index + step)
                        {
                            const auto adjusted = offset + static_cast<ssize_t>(validSlots[index]);
                            const auto& aliceQubit = aliceQubits[static_cast<size_t>(adjusted)];
                            const auto& bobQubit = receiverResults[index];

                            if(adjusted >= 0 && adjusted < static_cast<ssize_t>(aliceQubits.size()) &&
                               QubitHelper::Base(aliceQubit) == QubitHelper::Base(bobQubit))
                            {
                                basesMatched++;
                                if(aliceQubit == bobQubit)
                                {
                                   validCount++;
                                }
                            }
                        }
                        confidence = static_cast<double>(validCount) / basesMatched;
*/
                        const auto confidence = offsetting.CompareValues(aliceQubits, validSlots, receiverResults, offset);
                        //LOGDEBUG("Offset = " + to_string(offset) + ", confidence = " + to_string(confidence));

                        if(confidence > highestMatch)
                        {
                            highestMatch = confidence;
                            highestOffset = offset;
                            highestChannelMappings = channelMappings;
                        }
                        /*
                        QubitList aliceFiltered;
                        aliceFiltered.reserve(aliceQubits.size());
                        for(auto validSlot = validSlots.cbegin(); validSlot != validSlots.cend(); validSlot++)
                        {
                            const ssize_t index = *validSlot + offset;
                            if(index > 0 && index < aliceQubits.size())
                            {
                                aliceFiltered.push_back(aliceQubits[index]);
                            }
                        }

                        //const size_t numTestValues = aliceFiltered.size() / 100;
                        align::Gating::SparseQubitList testValues;
                        for(auto index = 0u; index < 1000; index++)
                        {
                            testValues[index] = aliceFiltered[index];
                        }

                        auto match = align::Gating::EstimateMatch(testValues, receiverResults);

                        if(match > highestMatch)
                        {
                            highestMatch = match;
                            highestOffset = offset;
                        }
                        */

                    }

                    // recalculate a more accurate confidence
                    highestMatch = align::Offsetting(100000, 0.0, 1.0).CompareValues(aliceQubits, validSlots, receiverResults, highestOffset);
                    LOGDEBUG("Highest match: " + to_string(highestMatch * 100) + "% at " + to_string(highestOffset) +
                             " [ " + to_string(highestChannelMappings[0]) + ", " +
                            to_string(highestChannelMappings[1]) + ", " +
                            to_string(highestChannelMappings[2]) + ", " +
                            to_string(highestChannelMappings[3]) + "]");
                }
            }
        }


    }

    return exitCode;
}

CQP_MAIN(QKDPostProc)
