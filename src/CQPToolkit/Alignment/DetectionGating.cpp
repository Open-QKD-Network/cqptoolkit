/*!
* @file
* @brief DetectionGating
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 19/9/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "DetectionGating.h"
#include <cmath>
#include <sstream>
#include "CQPToolkit/Util/OpenCLHelper.h"

extern const char _binary_Histogram_cl_start[];
extern const char _binary_Histogram_cl_end[];
const size_t Histogram_cl_size = static_cast<size_t>(_binary_Histogram_cl_end - _binary_Histogram_cl_start);

namespace cqp
{
    namespace align
    {
        DetectionGating::DetectionGating(IRandom& randGen) :
            randomGenerator(randGen) {

            SetNumberThreads(0);
            globalCounts.resize(numBins);
        }

        bool DetectionGating::SetSystemParameters(PicoSeconds newFrameWidth, PicoSeconds newSlotWidth, PicoSeconds newPulseWidth,
                                         uint64_t newSlotOffsetTestRange,
                                         double newAcceptanceRatio)
        {
            bool result = true;

            if(numSlots == 0 || slotWidth.count() == 0 || pulseWidth.count() == 0 || acceptanceRatio == 0.0 || acceptanceRatio == 1.0)
            {
                LOGERROR("Invalid parameters, algorithm will fail.");
                result = false;
            }
            numSlots = newFrameWidth / newSlotWidth;
            slotWidth = newSlotWidth;
            pulseWidth = newPulseWidth;
            acceptanceRatio = newAcceptanceRatio;
            numBins = newSlotWidth / newPulseWidth;
            slotOffsetTestRange = newSlotOffsetTestRange;

            globalCounts.resize(numBins);
            return result;
        }

        void DetectionGating::SetNumberThreads(uint threads)
        {
            if(threads == 0)
            {
                // hardware_concurrency returns 0 if it can't detect the numbers of threads
                // so always use at least 1
                maxThreads = std::max(std::thread::hardware_concurrency(), 1u);
            }
            else
            {
                maxThreads = threads;
            }
        }

        void DetectionGating::CountDetections(const DetectionGating::DetectionBounds& dataBounds,
                                              uint64_t numBins,
                                              const PicoSecondOffset& drift,
                                              const PicoSeconds& slotWidth,
                                              const PicoSeconds& pulseWidth,
                                              ResultsByBinBySlot& slotResults,
                                              CountsByBin& counts)
        {
            using namespace std;
            counts.resize(numBins);
            // for each detection
            //   calculate it's slot and bin ids and store a reference to the original data
            //   count the bin ids
            for(DetectionReportList::const_iterator detection = dataBounds.first; detection != dataBounds.second; ++detection)
            {
                using namespace std::chrono;
                // calculate the offset in whole picoseconds (signed)
                const PicoSecondOffset offset(static_cast<int64_t>(
                            ceil((static_cast<double>(drift.count()) * static_cast<double>(detection->time.count())) / 1000000000.0)
                            ));
                // offset the time without the original value being converted to a float
                const PicoSeconds adjustedTime = detection->time + offset;
                // C++ truncates on integer division
                const SlotID slot = adjustedTime / slotWidth;
                const BinID bin = (adjustedTime % slotWidth) / pulseWidth;
                // store the value as a bin for later access
                //slotResults[bin][slot].push_back(detection);
                // the value is atomic so we can safely update it
                counts[bin]++;
            }

        }

        DetectionGating::OffsetHighscore DetectionGating::ScoreOffsets(
                const std::pair<uint64_t, uint64_t> &myOffsetRange,
                const remote::QubitByIndex &markers,
                const DetectionGating::ValuesBySlot& allResults)
        {
            using namespace std;
            OffsetHighscore offsetHighscore;

            for (uint64_t testOffset = myOffsetRange.first; testOffset < myOffsetRange.second; testOffset++)
            {
                int score = 0;
                uint64_t markersFoundThisTime = 0;
                for(const auto& marker : markers.qubits())
                {
                    auto valueIt = allResults.find(marker.first + testOffset);
                    if(valueIt != allResults.end())
                    {
                        //LOGDEBUG("Checking marker " + to_string(marker.first));
                        markersFoundThisTime++;

                        for(auto& element : valueIt->second)
                        {
                            if(QubitHelper::Base(*element) == QubitHelper::Base(static_cast<Qubit>(marker.second)))
                            {
                                if(QubitHelper::BitValue(*element) == QubitHelper::BitValue(static_cast<Qubit>(marker.second)))
                                {
                                    // we guessed the basis right and it matched
                                    score++;
                                } else {
                                    // we guessed the basis right but it didn't match
                                    score--;
                                }
                            }
                        }
                    } else {
                        //LOGWARN("Marker not found:" + to_string(marker.first));
                    } // if marker found
                } // for marker qubit

                if(score > offsetHighscore.score)
                {
                    offsetHighscore.score = score;
                    offsetHighscore.slotIdOffset = testOffset;
                    LOGDEBUG("New high score, offset:" + std::to_string(offsetHighscore.slotIdOffset) +
                             ", score:" + std::to_string(offsetHighscore.score) +
                             ", markers:" + to_string(markersFoundThisTime));
                } // if new high score
            } // for offsets to test

            return offsetHighscore;
        } // ScoreOffsets

        void DetectionGating::HistogramWorker(const DetectionGating::DetectionBounds& dataBounds,
            std::pair<uint64_t, uint64_t> myOffsetRange,
            const remote::QubitByIndex& markers,
            OffsetHighscore& offsetHighscore)
        {
            using namespace std;
            LOGDEBUG("Running...");

            ResultsByBinBySlot myResults;
            CountsByBin myCounts;

            CountDetections(dataBounds, numBins, calculatedDrift, slotWidth, pulseWidth, myResults, myCounts);

            /*lock scope*/{
                // get a lock
                unique_lock<mutex> lock(threadStaging.summationMutex);
                // add our counts to the global list
                std::transform(myCounts.begin(), myCounts.end(), globalCounts.begin(), globalCounts.begin(), std::plus<CountsByBin::value_type>());
            }/*lock scope*/

            // decrement the counter to find out if the other threads are done
            if(threadStaging.threadsActive.fetch_sub(1) == 1) // true if we just set threadsActive to 0
            {
                // tell the parent thread it's time to find the correct bins
                threadStaging.summationCv.notify_all();
            }

            // === Parent thread calculates the correct bins === //

            /*lock scope*/{
                LOGDEBUG("Waiting for bin calculations...");
                // wait for the bin to be calculated
                unique_lock<mutex> lock(threadStaging.summationMutex);
                threadStaging.summationCv.wait(lock, [&]
                {
                    return threadStaging.targetBinFound;
                });

                auto binId = minBinId;
                do {
                    for(const auto& usableResults : myResults[binId])
                    {
                        for(auto detection : usableResults.second)
                        {
                            // add the result value to the list for it's slot
                            // multiple results will be randomly chosen later
                            allResults[usableResults.first].push_back(detection);
                        }
                    }
                    binId = (binId + 1) % numBins;
                } while(binId != ((maxBinId + 1) % numBins));

                // decrement the counter to find out if the other threads are done
                if(threadStaging.threadsActive.fetch_sub(1) == 1) // true if we just set threadsActive to 0
                {
                    threadStaging.resultsCollected = true;
                    // tell the other threads it's time to find the correct bins
                    threadStaging.summationCv.notify_all();
                }
            }/*lock scope*/

            // === other threads add their results to allResults === //

            /*lock scope*/{
                LOGDEBUG("Waiting for results to be collected...");
                // wait for results to be collected
                unique_lock<mutex> lock(threadStaging.summationMutex);
                threadStaging.summationCv.wait(lock, [&]
                {
                    return threadStaging.resultsCollected;
                });
            }/*lock scope*/

            // find a match between the markers and our data
            offsetHighscore = ScoreOffsets(myOffsetRange, markers, allResults);

            LOGDEBUG("Finished.");
        } // HistogramWorker

        grpc::Status DetectionGating::GetMarkers(std::shared_ptr<grpc::Channel> channel, SequenceNumber frameId, remote::QubitByIndex& results)
        {
            auto otherSide = remote::IAlignment::NewStub(channel);
            grpc::ClientContext ctx;
            remote::FrameId request;
            request.set_id(frameId);

            return LogStatus(otherSide->GetAlignmentMarks(&ctx, request, &results));
        } // GetMarkers

        void DetectionGating::CalculateDrift()
        {
            using namespace std;
            BinID targetBinId = 0;

            stringstream debugResults;

            for(uint64_t index = 0; index < globalCounts.size(); index++)
            {
                debugResults << globalCounts[index] << ",";
                if(globalCounts[index] > globalCounts[targetBinId])
                {
                    targetBinId = index;
                }
            } // for counts
            LOGDEBUG("Before drift: " + debugResults.str());

            minBinId = targetBinId;
            maxBinId = targetBinId;
            const uint64_t minCount = max(1ul, static_cast<uint64_t>(globalCounts[targetBinId] * acceptanceRatio));

            int16_t driftOffset = 0;
            for(uint64_t index = 1; index < numBins; index++)
            {
                const auto indexToCheck = (targetBinId + index) % numBins;
                // using wrapping, look right of the peek
                if(globalCounts[indexToCheck] >= minCount)
                {
                    // nudge the drift to the right
                    driftOffset++;
                    maxBinId = indexToCheck;
                } else {
                    break; // for
                }
            }

            for(uint64_t index = 1; index < numBins; index++)
            {
                const BinID indexToCheck = (numBins + targetBinId - index) % numBins;
                // find the first one which is too small
                if(globalCounts[indexToCheck] >= minCount)
                {
                    // nudge the drift to the left
                    driftOffset--;
                    minBinId = indexToCheck;
                } else {
                    break; // for
                }
            }

            // move the drift by half to make a small correction
            calculatedDrift = calculatedDrift + (pulseWidth * driftOffset) / 2;
            stringstream driftValue;
            driftValue << calculatedDrift.count();
            LOGDEBUG("Calculated drift offset:" + std::to_string(driftOffset) + " Drift: " + driftValue.str());
            LOGDEBUG("Min Bin: " + to_string(minBinId) + " target: " + to_string(targetBinId) + " max bin: " + to_string(maxBinId));
            if(minBinId == targetBinId + 1 && maxBinId == targetBinId - 1)
            {
                LOGERROR("All bins within drift tolerance. Noise level too high.");
            }

        } // CalculateDrift

        std::unique_ptr<QubitList> DetectionGating::BuildHistogram(const DetectionReportList& source, SequenceNumber frameId, std::shared_ptr<grpc::Channel> channel)
        {
            using namespace std;
            using std::vector;
            std::unique_ptr<QubitList> results(new QubitList);

            CountsByBin counts(numBins);

            // each thread will have at least one item to process
            // we may not use all the available threads
            const uint numThreads = static_cast<uint>(min(source.size(), static_cast<unsigned long>(maxThreads)));
            const uint64_t itemsPerThread = source.size() / numThreads;
            const uint64_t offsetsPerThread = slotOffsetTestRange / numThreads;

            std::lock_guard<std::mutex> processingLock(processingMutex);

            threadPool.clear();
            threadPool.resize(numThreads);
            threadStaging.threadsActive = numThreads;
            threadStaging.resultsCollected = false;
            threadStaging.targetBinFound = false;

            // clear any previous values
            std::fill(globalCounts.begin(), globalCounts.end(), 0);

            remote::QubitByIndex markers;
            LOGDEBUG("Starting " + to_string(numThreads) + " threads.");

            for(uint8_t threadId = 0; threadId < numThreads; threadId++)
            {
                const long offset = static_cast<long>(threadId * itemsPerThread);
                DetectionReportList::const_iterator start = source.begin() + offset;
                DetectionReportList::const_iterator end;
                std::pair<uint64_t, uint64_t> startOffsetRange;
                startOffsetRange.first = threadId * offsetsPerThread;

                if(threadId == numThreads - 1)
                {
                    // include any remainders in the last thread
                    end = source.end();
                    startOffsetRange.second = slotOffsetTestRange + 1;
                }
                else
                {
                    // end = one past the last item to process (same as end())
                    end = source.begin() + static_cast<long>(itemsPerThread) + offset;
                    startOffsetRange.second = startOffsetRange.first + offsetsPerThread;
                }

                DetectionGating::DetectionBounds bounds = {start, end};
                threadPool[threadId].theThread = thread(
                            &DetectionGating::HistogramWorker, this, bounds, startOffsetRange, ref(markers), ref(threadPool[threadId].highScoreOffset));

            } // for each thread

            // ask the other side for some points of reference to shift our slot index to line up with theirs
            if(!GetMarkers(channel, frameId, markers).ok() || markers.qubits().empty())
            {
                LOGERROR("Invalid markers provided");
            }

            {
                // get a lock on the count list and wait until all the threads are finished
                std::unique_lock<std::mutex> lock(threadStaging.summationMutex);
                threadStaging.summationCv.wait(lock, [&]
                {
                    return threadStaging.threadsActive == 0;
                });

                // find the bin range
                CalculateDrift();

                threadStaging.targetBinFound = true;
                threadStaging.resultsCollected = false;
                threadStaging.threadsActive = numThreads;
                threadStaging.summationCv.notify_all();
            }

            OffsetHighscore highestScore;
            // wait for threads to finish
            for(uint8_t threadId = 0 ; threadId < numThreads; threadId++)
            {
                threadPool[threadId].theThread.join();

                if(threadPool[threadId].highScoreOffset.score > highestScore.score)
                {
                    highestScore = threadPool[threadId].highScoreOffset;
                }
            } // for threads
            // now all the data has been written to allResults and the start slot has been decided

            LOGDEBUG("Using offset: " + std::to_string(highestScore.slotIdOffset));

            // build a list of slots which have been detected
            // it doesn't matter if we get two values for a slot at this point
            DetectedSlots detectedSlots;
            // collapse the sparse 2d array into a 1d array.
            // The slot id is thrown away at this point as the unsuccessfully detected slots are discarded

            if(allResults.size() > markers.qubits().size())
            {
                const auto usableResults = allResults.size() - markers.qubits().size();
                results->reserve(usableResults);

                for(const auto& detectionList : allResults)
                {
                    const auto correctedSlotId = detectionList.first + highestScore.slotIdOffset;
                    // drop the markers and detections past the end of transmission
                    if(correctedSlotId < numSlots &&
                       !detectionList.second.empty() &&
                       markers.qubits().find(correctedSlotId) == markers.qubits().end())
                    {
                        detectedSlots.insert(correctedSlotId);
                        if(detectionList.second.size() == 1)
                        {
                            // only one result to use
                            results->push_back(*detectionList.second[0]);
                        }
                        else
                        {
                            // choose one at random
                            const auto index = randomGenerator.RandULong() % detectionList.second.size();
                            results->push_back(*detectionList.second[index]);
                        }
                    }
                }
            } else {
                LOGWARN("No usable results in this data");
            }

            // pass the known slots to the other side so we can shorten the lists
            grpc::Status txResult = SendValidDetections(channel, frameId, detectedSlots, highestScore.slotIdOffset);
            allResults.clear();

            return results;

        } // BuildHistogram

        grpc::Status DetectionGating::SendValidDetections(
                std::shared_ptr<grpc::Channel> channel, SequenceNumber frame,
                const DetectionGating::DetectedSlots& results, uint64_t offset)
        {
            auto otherSide = remote::IAlignment::NewStub(channel);
            grpc::ClientContext ctx;
            remote::ValidDetections request;
            google::protobuf::Empty response;

            request.set_frameid(frame);
            for(auto slot : results)
            {
                request.add_slotids(slot + offset);
            }
            return LogStatus(otherSide->DiscardTransmissions(&ctx, request, &response));
        } // SendValidDetections

#if defined(OPENCL_FOUND)
        void CLTest() {
            LOGTRACE("Starting OpenCL");
            cl::Program::Sources sources;
            sources.push_back(std::string(_binary_Histogram_cl_start, Histogram_cl_size));
            cl::Platform platform;
            cl::Device device;
            cl_int errCode = CL_SUCCESS;
            QubitList receivedData;

            if(OpenCLHelper::FindBestDevice(platform, device))
            {
                cl::Context* clCtx = new cl::Context(device);
                cl::Program program(sources, &errCode);
                if(OpenCLHelper::LogCL(errCode) == CL_SUCCESS)
                {
                    if(OpenCLHelper::LogCL(program.build({device})) != CL_SUCCESS)
                    {
                        LOGERROR(" Error building: " + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) );
                    }
                }

                if(errCode == CL_SUCCESS)
                {

                    cl::Kernel* myKernel = new cl::Kernel(program, "histogram", &errCode);
                    if(OpenCLHelper::LogCL(errCode) == CL_SUCCESS)
                    {
                        cl::Buffer inputData(*clCtx, CL_MEM_READ_ONLY, sizeof(ProtocolDetectionReport) * receivedData.size());
                        cl::Buffer slotWidthData(*clCtx, CL_MEM_READ_ONLY, sizeof(uint64_t));
                        cl::Buffer peakSlot(*clCtx, CL_MEM_READ_WRITE, sizeof(uint64_t));

                        cl::CommandQueue queue(*clCtx, device);
                        queue.enqueueWriteBuffer(inputData, CL_TRUE, 0, sizeof(ProtocolDetectionReport) * receivedData.size(), receivedData.data());

                        myKernel->setArg(0, inputData);
                        myKernel->setArg(1, slotWidthData);
                        myKernel->setArg(2, peakSlot);

                        queue.enqueueNDRangeKernel(*myKernel, cl::NullRange, cl::NDRange(receivedData.size()), cl::NullRange);
                        queue.finish();

                        uint64_t result = 0;
                        queue.enqueueReadBuffer(peakSlot, CL_TRUE, 0, sizeof (result), &result);

                    }
                    delete myKernel;
                }
                delete clCtx;
            }
            else
            {
                LOGERROR("OpenCL not available");
            }
        }
#endif // OPENCL_FOUND

    } // namespace align
} // namespace cqp
