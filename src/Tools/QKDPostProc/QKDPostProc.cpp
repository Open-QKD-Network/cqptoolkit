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
    static CONSTSTRING findDrift= "find-drift";
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
    definedArguments.AddOption(Names::findDrift, "d", "Search the detections for a drift value")
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

void QKDPostProc::FindDrift(const DetectionTimes& detections)
{
    uint64_t maxDrift {  40000};
    uint32_t driftStep {   100};
    uint32_t numBins   {   100};
    uint64_t pulseWidth { 1000};
    uint64_t slotWidth {100000};

    definedArguments.GetProp(Names::driftMax, maxDrift);
    definedArguments.GetProp(Names::driftStep, driftStep);
    definedArguments.GetProp(Names::bins, numBins);
    definedArguments.GetProp(Names::pulseWidth, pulseWidth);
    definedArguments.GetProp(Names::slotWidth, slotWidth);

    const uint maxThreads { std::max(1u, std::thread::hardware_concurrency()) };

    align::Filter filter;
    DetectionTimes::const_iterator start;
    DetectionTimes::const_iterator end;
    filter.Isolate(detections, start, end);
    LOGINFO("Start: " + std::to_string(std::distance(detections.begin(), start)) + " End: " + std::to_string(std::distance(detections.begin(), end)));

    ////////////////////////////////////////
    /*
    std::vector<PicoSeconds> diffs;
    diffs.resize(detections.size());
    const uint itemsPerThread = std::max(1u, static_cast<uint>(detections.size() / maxThreads));

    std::vector<std::future<void>> diffFutures;
    for(uint threadId = 0; threadId < maxThreads; threadId++)
    {
        diffFutures.push_back(std::async(std::launch::async, [&](){
            const auto startPoint = threadId * itemsPerThread;
            for(uint index = startPoint; (index < startPoint + itemsPerThread) && index < detections.size() - 1; index++)
            {
                diffs[index] = detections[index + 1].time - detections[index].time;
            }
        }));
    }

    for(auto& fut : diffFutures)
    {
        fut.wait();
    }
    diffFutures.clear();


    using myFutures = std::future<RunData>;
    std::vector<myFutures> futures;
    std::condition_variable threadLimit;

    RunData winner;


    for(auto offset = PicoSecondOffset(-1l * static_cast<int64_t>(maxDrift)) ;
        offset <= PicoSecondOffset(maxDrift);
        offset += PicoSecondOffset(driftStep))
    {

        while(futures.size() >= maxThreads) {

            for(auto future = futures.begin() ; future != futures.end(); future++)
            {
                if(future->valid() && future->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    auto result = future->get();
                    if(result.highestValue > winner.highestValue)
                    {
                        winner = result;
                    }
                    futures.erase(future);
                    break;
                }
            }

        }

        //futures.push_back(std::async(std::launch::async, DoDrift, offset,
        //                             std::ref(detections), numBins, PicoSeconds(slotWidth), PicoSeconds(pulseWidth)));
    }

    for(auto& future : futures)
    {
        auto result = future.get();
        if(result.highestValue > winner.highestValue)
        {
            winner = result;
        }
    }

    LOGINFO("The winner is: Offset " + std::to_string(winner.offset.count()) + " Highest value = " + std::to_string(winner.highestValue) + " @ " + std::to_string(winner.highestIndex));
    */
}

int QKDPostProc::Main(const std::vector<std::string>& args)
{
    using namespace cqp;
    using namespace std;
    exitCode = Application::Main(args);

    if(!stopExecution)
    {
        std::string detectionsFile = "BobDetections.bin";
        DetectionReports detections;
        definedArguments.GetProp(Names::noxData, detectionsFile);

        if(!fs::DataFile::ReadNOXDetections(detectionsFile, detections))
        {
            LOGERROR("Failed to open file: " + detectionsFile);
        }

        bool findDrift = true;
        definedArguments.GetProp(Names::findDrift, findDrift);
        if(detections.times.size() > 0 &&  findDrift)
        {
            FindDrift(detections.times);
        }
    }

    return exitCode;
}

CQP_MAIN(QKDPostProc)