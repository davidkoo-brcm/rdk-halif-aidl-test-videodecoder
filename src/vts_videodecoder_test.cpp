/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 */

/**
 * @addtogroup VIDEO_DECODER Video Decoder Module
 * @{
 */

/**
 * @addtogroup VIDEO_DECODER_HALTEST Video Decoder HAL Tests
 * @{
 */

/**
 * @file vts_videodecoder_test.cpp
 *
 * @brief Contains the videoDecoderTest class implementation and frame parsing logic for video decoder testing.
 */

#include "vts_videodecoder.h"
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <chrono>
#include <filesystem>
#include <unistd.h> // For getpid()
#include <sys/syscall.h> // For syscall(SYS_gettid)

using namespace std::chrono;
using namespace std;

// AVBufferSpaceListener implementation
Status AVBufferSpaceListener::onSpaceAvailable()
{
    return Status::ok();
}

// videoDecoderTest method implementations
void videoDecoderTest::run(bool concurrentDecodeConsume)
{
    UT_LOG_DEBUG("concurrentDecodeConsume %d ",concurrentDecodeConsume);

    thread consumerThread;

    mFrameData.clear();
    mInputBuffer.clear();

    bool status = makeFrame();
    if (!status)
    {
        UT_LOG_ERROR("Failed to make frame data");
        return;
    }

    if (concurrentDecodeConsume)
    {
        UT_LOG_DEBUG("Start decoderThreadFunc & consumerThreadFunc");

        if(!mIsSinkAttached)
        {
            // If sink is not attached, we need to validate decoded frames in a separate thread
            consumerThread = thread(&videoDecoderTest::validateDecodedFrame, this);
        }
        setupVideoDecoder();

        startDecoding();

        if(!mIsSinkAttached && consumerThread.joinable())
        {
            // If sink is not attached, we need to validate decoded frames here
            //validateDecodedFrame();
            consumerThread.join();
        }

        teardownDecoder();
    }
    else
    {
        decodeAndConsumeVideoFrames();
    }
}

void videoDecoderTest::getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped)
{
    optional<PropertyValue> propertyValue;

    framesDecoded = decodeErrors = framesDropped = -1;

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DECODED, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_DECODE_ERRORS, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DROPPED, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }
}

void videoDecoderTest::setupVideoDecoder()
{
    mBufferListener = new AVBufferSpaceListener();

    // Create Video Pool
    AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, mBufferListener, mVideoPool);

    // Open Decoder
    VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode,
                              mControlListener, mVideoDecoderController, true);

    if (mTunnelMode)
    {
        std::vector<std::pair<videodecoder::Property, int32_t>> startedIntProps = {
            {videodecoder::Property::OPERATIONAL_MODE, static_cast<int32_t>(OperationalMode::TUNNELLED)}
        };
        UT_LOG_DEBUG("Set properties in TUNNELLED Mode");
        VIDEODECODER_SETPROPERTY_LIST_SUCCESS(mVideoDecoderController, startedIntProps, intValue);
    }
    if (mTestConfiguration.codedWidth > 0 && mTestConfiguration.codedHeight > 0)
    {
        VIDEODECODER_SETSTREAMRESOLUTION_SUCCESS(mVideoDecoderController, mTestConfiguration.codedWidth, mTestConfiguration.codedHeight);
    }
    if (mTestConfiguration.frameRateDenominator > 0)
    {
        VIDEODECODER_SETFRAMERATE_SUCCESS(mVideoDecoderController, mTestConfiguration.frameRateNumerator, mTestConfiguration.frameRateDenominator);
    }
    // Start Decoder
    VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);

    UT_LOG_DEBUG("Decoder started");
}

void videoDecoderTest::startDecoding()
{
    int64_t videoBufferHandle = 0;

    UT_LOG_DEBUG("[PID: %d, TID: %d] startDecoding Start", getpid(), syscall(SYS_gettid));

    // Process Frames
    for (size_t i = 0; i < mFrameData.size(); ++i)
    {
        auto [frameOffset, frameSize, framePTS, isErrorFrame] = mFrameData[i];

        UT_LOG_INFO("Processing frame %zu: offset=%llu, size=%u, PTS=%lldns, errorFrame=%d",
                    i, frameOffset, frameSize, framePTS, isErrorFrame);

        // Allocate Buffer
        videoBufferHandle = 0;
        AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mVideoPool, frameSize, videoBufferHandle);

        // Write Frame Data
        void* mapBuffer = nullptr;
        uint32_t mapSize = 0;
        if (mSecureDecode)
        {
            UT_LOG_INFO("Calling writeSecureHandle(handle:0x%llx, size=%u, mAVbufferHelper:%p)",
                        videoBufferHandle, frameSize, mAVbufferHelper);
            if (!mAVbufferHelper->writeSecureHandle(videoBufferHandle, mInputBuffer.data() + frameOffset, frameSize))
            {
                UT_LOG_ERROR("Failed to copy frame data to secure buffer");
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                continue;
            }
        }
        else
        {
            UT_LOG_INFO("Calling mapHandle(handle:0x%llx, mAVbufferHelper:%p)", videoBufferHandle, mAVbufferHelper);
            if (!mAVbufferHelper)
            {
                UT_LOG_ERROR("mAVbufferHelper is null before mapHandle");
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                continue;
            }
            mapBuffer = mAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
            if (mapBuffer && mapSize >= frameSize)
            {
                UT_LOG_DEBUG("Copying frame data to mapped buffer, size=%u", frameSize);
                memcpy(mapBuffer, mInputBuffer.data() + frameOffset, frameSize);
            }
            else
            {
                UT_LOG_ERROR("Failed to map buffer or insufficient size (mapBuffer:%p, mapSize:%u, required:%u)",
                            mapBuffer, mapSize, frameSize);
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                continue;
            }
        }

        // Decoder Operations
        if (mTestConfiguration.stopPts == framePTS)
        {
            UT_LOG_INFO("Stopping decoder at PTS:%lld", framePTS);
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
            VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
        if (mTestConfiguration.discontinuityPts == framePTS)
        {
            UT_LOG_INFO("Signaling discontinuity at PTS:%lld", framePTS);
            VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
        if (mTestConfiguration.flushPts == framePTS)
        {
            UT_LOG_INFO("Flushing decoder at PTS:%lld", framePTS);
            VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, mVideoDecoderController, false, true);
            VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
        if (mTestConfiguration.flushResetPts == framePTS)
        {
            UT_LOG_INFO("Flush reset at PTS:%lld", framePTS);
            VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, mVideoDecoderController, true, true);
            VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }

        if (mTestConfiguration.eosPts == framePTS)
        {
            UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
            VIDEODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
            VIDEODECODER_DECODEBUFFER_FAILURE(mVideoDecoderController, framePTS, videoBufferHandle, Status::EX_NONE);
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
            VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }

        UT_LOG_DEBUG("Adding frame to mInputFrame, PTS:%lld, errorFrame=%d", framePTS, isErrorFrame);

        // Decode Frame
        InputVideoFrame_t inputVideo;
        inputVideo.framePTS = framePTS;
        inputVideo.rms_threshold = mTestConfiguration.rms_diff_threshold;
        inputVideo.isErrorFrame = isErrorFrame;
        inputVideo.sendTime = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(mInputFrameMutex);
            mInputFrame.push_back(inputVideo);
        }

        VIDEODECODER_DECODEBUFFER_SUCCESS(mVideoDecoderController, framePTS, videoBufferHandle);

        std::this_thread::sleep_for(std::chrono::milliseconds(mTestConfiguration.decoderDelay));

        if(i == (mFrameData.size() - 1) &&  mTestConfiguration.eosPts == -1)
        {
            VIDEODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
    }

    UT_LOG_DEBUG("[PID: %d, TID: %d] startDecoding End", getpid(), syscall(SYS_gettid));
}

bool videoDecoderTest::validateDecodedFrame()
{
    DecodedVideoFrame_t decodedFrame;
    // Initialize PTS tracking
    std::set<int64_t> receivedPts(mTestConfiguration.expectedPts.begin(), mTestConfiguration.expectedPts.end());
    bool isBufferValid = false;
    int32_t metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped;
    int32_t totalOnFramesDecoded = 0;
    int32_t totalOnErrorsReceived = 0;
    int32_t eosFramesReceived = 0;
    int32_t discontinuityFramesReceived = 0;

    while (mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(5000)))
    {
        UT_LOG_DEBUG("Retrieved remaining output frame: PTS=%lld, handle=0x%llx, isMetaAvailable=%d",
                decodedFrame.framePTS, decodedFrame.frameBufferHandle, decodedFrame.isMetaAvailable);

        // Validate PTS
        if(decodedFrame.framePTS != -1)
        {
            auto it = receivedPts.find(decodedFrame.framePTS);
            if (it == receivedPts.end())
            {
                UT_LOG_DEBUG("Unexpected PTS received: %lld", decodedFrame.framePTS);
                UT_ASSERT_TRUE(false);
                continue;
            }
            else
            {
                UT_LOG_DEBUG("Validated PTS: %lld", decodedFrame.framePTS);
                receivedPts.erase(it); // Remove to track received PTS
            }

            {
                std::lock_guard<std::mutex> lock(mInputFrameMutex);
                // Latency calculation: find matching input frame by PTS
                auto inputIt = std::find_if(mInputFrame.begin(), mInputFrame.end(),
                    [&decodedFrame](const InputVideoFrame_t& inputFrame) {
                        return inputFrame.framePTS == decodedFrame.framePTS;
                    });
                if (inputIt != mInputFrame.end())
                {
                    auto sendTime = inputIt->sendTime;
                    auto receiveTime = decodedFrame.receiveTime;
                    auto decodeTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(receiveTime - sendTime).count();
                    if (decodeTimeNs > mTestConfiguration.decode_time_threshold)
                    {
                        UT_LOG_WARNING("Frame PTS:%lld decode time: %lld ns exceeds threshold %lld ns",
                                    decodedFrame.framePTS, decodeTimeNs, mTestConfiguration.decode_time_threshold);
                    }
                }
            }


            if(decodedFrame.frameBufferHandle != -1)
            {
                //TODO: Need to validate buffer content as well. Currently just checking if buffer handle is valid and freeing it.
                AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle, isBufferValid);
                if (isBufferValid)
                {
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
                }
            }
            totalOnFramesDecoded++;
        }
        if (decodedFrame.metadata)
        {
            UT_LOG_DEBUG("Validating metadata for PTS=%lld", decodedFrame.framePTS);
            // Validate resolution
            UT_ASSERT_EQUAL(decodedFrame.metadata->codedWidth, mTestConfiguration.codedWidth)
                << "Width mismatch: expected=" << mTestConfiguration.codedWidth
                << ", actual=" << decodedFrame.metadata->codedWidth
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
            UT_ASSERT_EQUAL(decodedFrame.metadata->codedHeight, mTestConfiguration.codedHeight)
                << "Height mismatch: expected=" << mTestConfiguration.codedHeight
                << ", actual=" << decodedFrame.metadata->codedHeight
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

            UT_ASSERT_EQUAL(decodedFrame.metadata->colorDepth, mTestConfiguration.colorDepth)
                << "Color depth mismatch: expected=" << mTestConfiguration.colorDepth
                << ", actual=" << decodedFrame.metadata->colorDepth
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

            // Validate dynamic range
            UT_ASSERT_EQUAL(decodedFrame.metadata->dynamicRange, mTestConfiguration.dynamicRange)
                << "Dynamic range mismatch: expected=" << static_cast<int>(mTestConfiguration.dynamicRange)
                << ", actual=" << static_cast<int>(decodedFrame.metadata->dynamicRange)
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

            // Validate colorimetry
            UT_ASSERT_EQUAL(decodedFrame.metadata->colorimetry, mTestConfiguration.colorimetry)
                << "Colorimetry mismatch: expected=" << static_cast<int>(mTestConfiguration.colorimetry)
                << ", actual=" << static_cast<int>(decodedFrame.metadata->colorimetry)
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

            // Validate frame rate
            UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateNumerator, mTestConfiguration.frameRateNumerator)
                << "Frame rate numerator mismatch: expected=" << mTestConfiguration.frameRateNumerator
                << ", actual=" << decodedFrame.metadata->frameRateNumerator
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
            UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateDenominator, mTestConfiguration.frameRateDenominator)
                << "Frame rate denominator mismatch: expected=" << mTestConfiguration.frameRateDenominator
                << ", actual=" << decodedFrame.metadata->frameRateDenominator
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

            // Validate scan type
            UT_ASSERT_EQUAL(decodedFrame.metadata->scanType, mTestConfiguration.scanType)
                << "Scan type mismatch: expected=" << static_cast<int>(mTestConfiguration.scanType)
                << ", actual=" << static_cast<int>(decodedFrame.metadata->scanType)
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

            // Validate HDR metadata
            UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo, mTestConfiguration.masteringDisplayInfo)
                << "MasteringDisplayInfo mismatch for Codec "
                << videodecoder::toString(mTestConfiguration.codec).c_str();

            UT_ASSERT_EQUAL(decodedFrame.metadata->contentLightLevel, mTestConfiguration.contentLightLevel)
                << "ContentLightLevel mismatch for Codec "
                << videodecoder::toString(mTestConfiguration.codec).c_str();

            if (decodedFrame.metadata->endOfStream)
            {
                eosFramesReceived++;
            }

            if(decodedFrame.metadata->discontinuity)
            {
                discontinuityFramesReceived++;
            }

            UT_ASSERT_EQUAL(decodedFrame.metadata->lowLatency, mTestConfiguration.lowLatency)
                << "lowLatency mismatch: expected=" << mTestConfiguration.lowLatency
                << ", actual=" << decodedFrame.metadata->lowLatency
                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
        }
        else
        {
            UT_LOG_INFO("No metadata available for PTS=%lld", decodedFrame.framePTS);
        }

    }

    totalOnErrorsReceived = mEventListener->getErrorCount();
    if((mTestConfiguration.flushResetPts == -1) && (mTestConfiguration.flushPts==-1))
    {
        int32_t expectedFrames = mTestConfiguration.no_of_frames - mTestConfiguration.decodeErrors;

        // Get final metrics
        getDecoderMetrics(metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped);
        UT_LOG_DEBUG("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                     mTestConfiguration.codec, metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped);

        if(metricsFramesDecoded != -1)
        {
            UT_ASSERT_EQUAL(metricsFramesDecoded, expectedFrames);
        }
        UT_ASSERT_EQUAL(totalOnFramesDecoded, expectedFrames);

        if(metricsDecodeErrors != -1)
        {
            UT_ASSERT_EQUAL(metricsDecodeErrors, mTestConfiguration.decodeErrors);
        }
        UT_ASSERT_EQUAL(totalOnErrorsReceived, mTestConfiguration.decodeErrors);

        if(metricsFramesDropped != -1)
        {
            UT_ASSERT_EQUAL(metricsFramesDropped, mTestConfiguration.framesDropped);
        }
    }

    return true;
}

void videoDecoderTest:: teardownDecoder()
{
    // Stop Decoder
    VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);

    // Close Decoder
    VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, mVideoDecoderController, true);

    // Destroy Video Pool
    AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mVideoPool);
}

void videoDecoderTest::decodeAndConsumeVideoFrames()
{
    sp<IVideoDecoderController> mVideoDecoderController;
    int64_t videoBufferHandle;
    Pool mVideoPool;
    sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

    UT_LOG_DEBUG("decodeAndConsumeVideoFrames start:");

    AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, mVideoPool);

    ifstream inputFile(mTestConfiguration.input, ios::binary);
    if (!inputFile)
    {
        UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
        return;
    }

    VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, mVideoDecoderController, true);

    VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);

    for (const auto& [frameSize, framePTS, errorFrame] : mTestConfiguration.frameDetails)
    {
        vector<uint8_t> inputBuffer;
        DecodedVideoFrame_t decodedFrame;

        AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mVideoPool, frameSize, videoBufferHandle);

        inputBuffer.resize(frameSize);
        inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

        auto startTime = chrono::steady_clock::now();
        VIDEODECODER_DECODEBUFFER_SUCCESS(mVideoDecoderController, framePTS, videoBufferHandle);

        mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(100));

        if (!mTunnelMode && !decodedFrame.data.empty())
        {
            UT_LOG_INFO("Validating frame data for PTS=%lld", decodedFrame.framePTS);
        }

        auto duration = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - startTime);
        UT_LOG_INFO("Decode Time [PTS: %lld]: %lld", framePTS, duration.count());

        bool isValidBuffer = false;
        UT_LOG_INFO(" Calling isValid(IN:bufferHandle:[0X%016"PRIx64"], OUT:_aidl_return[])", videoBufferHandle);
        Status status = mAVBufferManager->isValid(videoBufferHandle, &isValidBuffer);
        UT_LOG_INFO(" Result isValid(IN:bufferHandle:[0X%016"PRIx64"], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
            videoBufferHandle, UT_Control_GetMapString(bool_mapTable, isValidBuffer), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        if(isValidBuffer)
        {
            UT_LOG_ERROR(" Encoded buffer not released by deecoder [handle:0x%llx]", videoBufferHandle);
        }
    }

    VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);

    VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, mVideoDecoderController, true);

    PoolMetrics poolMetrics;
    UT_LOG_INFO("Calling getPoolMetrics(IN:poolHandle:[%d], OUT:PoolMetrics[])", mVideoPool.handle); \
    Status status = mAVBufferManager->getPoolMetrics(mVideoPool, &poolMetrics); \
    UT_LOG_INFO("Result getPoolMetrics(IN:poolHandle:[%d], OUT:PoolMetrics[bytesUsed:%d, bytesTotal:%d]), status:[isOK:%s, exception:%s]", \
                mVideoPool.handle, poolMetrics.bytesUsed, poolMetrics.bytesTotal, \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \

    AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mVideoPool);
}

/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK
