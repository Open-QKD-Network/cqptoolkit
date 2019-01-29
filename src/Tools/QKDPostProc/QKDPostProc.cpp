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
#include "Algorithms/Util/ProcessingQueue.h"
#include "Algorithms/Util/RangeProcessing.h"

#include <thread>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <future>

using namespace cqp;

struct Names
{
    static CONSTSTRING bobData= "bobdata";
    static CONSTSTRING aliceQubits= "alice";
    static CONSTSTRING slotWidth = "slot-width";
    static CONSTSTRING pulseWidth = "pulse-width";
    static CONSTSTRING acceptanceRatio = "acceptance";
    static CONSTSTRING windowStart = "window-start";
    static CONSTSTRING windowEnd = "window-end";
    static CONSTSTRING driftPreset = "drift";
    static CONSTSTRING driftSamples = "drift-sample";
    static CONSTSTRING filterSigma = "filter-sigma";
    static CONSTSTRING filterWidth = "filter-width";
    static CONSTSTRING filterCourseCutoff = "filter-course";
    static CONSTSTRING filterFineCutoff = "filter-fine";
    static CONSTSTRING filterStride = "filter-stride";
    static CONSTSTRING rawOut = "out";
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

    definedArguments.AddOption(Names::bobData, "b", "Read Bob Detections from file")
            .Bind();
    definedArguments.AddOption(Names::aliceQubits, "a", "Read transmissions from packed Qubits file")
            .Bind();
    definedArguments.AddOption(Names::slotWidth, "w", "Slot width of transmissions in time*")
            .Bind();
    definedArguments.AddOption(Names::pulseWidth, "j", "Pulse width/Jitter of photon in time*")
            .Bind();
    definedArguments.AddOption(Names::acceptanceRatio, "r", "Value between 0 and 1 for the gating filter")
            .Bind();
    definedArguments.AddOption(Names::windowStart, "i", "Force isolation window start to this detection number")
            .Bind();
    definedArguments.AddOption(Names::windowEnd, "I", "Force isolation window end to this detection number")
            .Bind();
    definedArguments.AddOption(Names::driftPreset, "d", "Force a value for drift in time*")
            .Bind();
    definedArguments.AddOption(Names::driftSamples, "D", "Sample time for calculating drift*")
            .Bind();
    definedArguments.AddOption(Names::filterSigma, "s", "Sigma value for the gaussian filter")
            .Bind();
    definedArguments.AddOption(Names::filterWidth, "g", "Integer width in gaussian filter")
            .Bind();
    definedArguments.AddOption(Names::filterCourseCutoff, "c", "Percentage for first filter pass acceptance")
            .Bind();
    definedArguments.AddOption(Names::filterFineCutoff, "C", "Percentage for last filter pass acceptance")
            .Bind();
    definedArguments.AddOption(Names::filterStride, "S", "Data items to skip when filtering")
            .Bind();
    definedArguments.AddOption(Names::rawOut, "o", "Output final raw qubits to file")
            .Bind();
}

void QKDPostProc::DisplayHelp(const CommandArgs::Option&)
{
    definedArguments.PrintHelp(std::cout, "Processes QKD data using different parameters to produce key.",
                               "Note: Time values are integers and can have s, ms, ns, ps, fs, as suffix. No suffix is assumed to be picoseconds");
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
        definedArguments.GetProp(Names::bobData, detectionsFile);

        PicoSeconds slotWidth = std::chrono::nanoseconds(100);
        definedArguments.GetProp(Names::slotWidth, slotWidth);

        PicoSeconds pulseWidth = std::chrono::nanoseconds(1);
        definedArguments.GetProp(Names::pulseWidth, pulseWidth);

        auto acceptanceRatio = align::Gating::DefaultAcceptanceRatio;
        definedArguments.GetProp(Names::acceptanceRatio, acceptanceRatio);

        auto driftSampleTime = align::Drift::DefaultDriftSampleTime;
        definedArguments.GetProp(Names::driftSamples, driftSampleTime);

        auto filterSigma = align::Filter::DefaultSigma;
        definedArguments.GetProp(Names::filterSigma, filterSigma);

        auto filterWidth = align::Filter::DefaultFilterWidth;
        definedArguments.GetProp(Names::filterWidth, filterWidth);

        auto filterCourseThresh = align::Filter::DefaultCourseTheshold;
        definedArguments.GetProp(Names::filterCourseCutoff, filterCourseThresh);

        auto filterFineThresh = align::Filter::DefaultFineTheshold;
        definedArguments.GetProp(Names::filterFineCutoff, filterFineThresh);

        auto filterStride = align::Filter::DefaultStride;
        definedArguments.GetProp(Names::filterStride, filterStride);

        auto mapping = 0u;
        detections.clear();
        LOGDEBUG("Mapping: " + to_string(mapping++));
        if(!fs::DataFile::ReadNOXDetections(detectionsFile, detections))
        {
            LOGERROR("Failed to open file: " + detectionsFile);
        }else {
            // to isolate the transmission
            align::Filter filter(filterSigma, filterWidth, filterCourseThresh, filterFineThresh, filterStride);
            // to find the qubits
            align::Gating gating(rng, slotWidth, pulseWidth, acceptanceRatio);
            align::Drift drift(slotWidth, pulseWidth, driftSampleTime);
            // bobs qubits
            QubitList receiverResults;
            DetectionReportList::const_iterator start;
            DetectionReportList::const_iterator end;

            if(!definedArguments.HasProp(Names::windowStart) || !definedArguments.HasProp(Names::windowEnd))
            {
                const auto startTime = std::chrono::high_resolution_clock::now();
                filter.Isolate(detections, start, end);
                cout << "Window Start = " << distance(detections.cbegin(), start) << "\n";
                cout << "Window End = " << distance(detections.cbegin(), end) << "\n";
                const auto windowTime = std::chrono::high_resolution_clock::now() - startTime;
                cout << "Window Processing = " << std::to_string(chrono::duration_cast<SecondsDouble>(windowTime).count()) << "s" << "\n";
            }

            size_t startOffset = 0;
            if(definedArguments.GetProp(Names::windowStart, startOffset))
            {
                LOGWARN("Forcing window start to " + to_string(startOffset));
                start = detections.cbegin() + startOffset;
            }

            size_t endOffset = 0;
            if(definedArguments.GetProp(Names::windowEnd, endOffset))
            {
                LOGWARN("Forcing window end to " + to_string(startOffset));
                end = detections.cbegin() + endOffset;
            }

            cout << "Window Qubits = " << distance(start, end) << "\n";
            const auto window = (end - 1)->time - start->time;
            LOGINFO("Detections: " + std::to_string(distance(start, end)) + "\n" +
                    " Start: " + std::to_string(distance(detections.cbegin(), start)) + " @ " + to_string(start->time.count()) + "pS\n" +
                    " End: " + std::to_string(distance(detections.cbegin(), end)) + " @ " + to_string(end->time.count()) + "pS\n" +
                    " Duration: " + std::to_string(chrono::duration_cast<SecondsDouble>(window).count()) + "s");

            align::Gating::ValidSlots validSlots;

            AttoSecondOffset driftValue {0};
            if(!definedArguments.GetProp(Names::driftPreset, driftValue))
            {
                const auto startTime = std::chrono::high_resolution_clock::now();
                // drift was not specified, calculate it
                driftValue = drift.Calculate(start, end);
                const auto driftProcessing = std::chrono::high_resolution_clock::now() - startTime;
                cout << "Drift Value = " << driftValue.count() << "as\n";
                cout << "Drift Processing = " << std::to_string(chrono::duration_cast<SecondsDouble>(driftProcessing).count()) << "s" << "\n";
            }
            gating.SetDrift(driftValue);

            {
                const auto startTime = std::chrono::high_resolution_clock::now();
                gating.ExtractQubits(start, end, validSlots, receiverResults);
                const auto extractTime = std::chrono::high_resolution_clock::now() - startTime;
                cout << "Extract Qubits = " << receiverResults.size() << "\n";
                cout << "Extract Processing = " << std::to_string(chrono::duration_cast<SecondsDouble>(extractTime).count()) << "s" << "\n";

                LOGINFO("Found " + to_string(receiverResults.size()) + " Qubits, last slot ID: " + to_string(*validSlots.rbegin()) + ". Took " +
                        to_string(chrono::duration_cast<chrono::milliseconds>(extractTime).count()) + "ms");

            }

            ////////////// Load Alice data to compare with Bobs
            std::string packedFile = "AliceRandom.bin";
            definedArguments.GetProp(Names::aliceQubits, packedFile);


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
                { 0, 1, 3, 2 },
                { 3, 1, 0, 2 },
                { 1, 3, 0, 2 },*/
                { 0, 3, 1, 2 }//,
                //{ 3, 0, 1, 2 }
            }))
                {
                QubitList aliceQubits;
                LOGDEBUG("Loading Alice data file");
                if(!fs::DataFile::ReadPackedQubits(packedFile, aliceQubits, 0, channelMappings))
                {
                    LOGERROR("Failed to open transmitter file: " + packedFile);
                } else {

                    align::Offsetting offsetting(10000);

                    const auto startTime = std::chrono::high_resolution_clock::now();
                    align::Offsetting::Confidence highest = offsetting.HighestValue(aliceQubits, validSlots, receiverResults, 0, 8000);

                    const auto offsettingTime = std::chrono::high_resolution_clock::now() - startTime;
                    cout << "Offsetting Offset = " << highest.offset << "\n";
                    cout << "Offsetting Confidence = " << highest.value << "\n";
                    cout << "Offsetting Processing = " << std::to_string(chrono::duration_cast<SecondsDouble>(offsettingTime).count()) << "s" << "\n";

                    LOGDEBUG("Highest match: " + to_string(highest.value * 100) + "% at " + to_string(highest.offset) +
                             " [ " + to_string(channelMappings[0]) + ", " +
                            to_string(channelMappings[1]) + ", " +
                            to_string(channelMappings[2]) + ", " +
                            to_string(channelMappings[3]) + "]");

                    if(definedArguments.HasProp(Names::rawOut))
                    {
                        QubitList validBits;
                        for(auto index = 0u; index < validSlots.size(); index++)
                        {
                            const auto aliceSlot = validSlots[index] + highest.offset;
                            if(aliceSlot > 0 && aliceSlot < aliceQubits.size())
                            {
                                if(QubitHelper::Base(aliceQubits[aliceSlot]) == QubitHelper::Base(receiverResults[index]))
                                {
                                    validBits.push_back(receiverResults[index]);
                                }
                            }
                        }
                        fs::DataFile::WriteQubits(validBits, definedArguments.GetStringProp(Names::rawOut));
                    }

                }
            }
        }


    }

    return exitCode;
}

CQP_MAIN(QKDPostProc)
