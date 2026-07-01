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
    thread consumerThread;

    if (!mAVbufferHelper)
    {
        UT_ASSERT_TRUE(false) << "AVBufferHelper is not initialized";
        return;
    }

    mFrameData.clear();
    mInputBuffer.clear();

    bool status = makeFrame();
    if (!status)
    {
        UT_ASSERT_TRUE(false) << "Failed to prepare frame data for decoding";
        return;
    }

    if (concurrentDecodeConsume)
    {
        if(!mIsSinkAttached)
        {
            // If sink is not attached, we need to validate decoded frames in a separate thread
            consumerThread = thread(&videoDecoderTest::validateDecodedFrame, this);
        }
        setupVideoDecoder();

        if(mVideoDecoderController.get() == nullptr)
        {
            UT_ASSERT_TRUE(false) << "Decoder session is not available";
            if(!mIsSinkAttached && consumerThread.joinable())
            {
                consumerThread.join();
            }
            return;
        }

        startDecoding();

        if(!mIsSinkAttached && consumerThread.joinable())
        {
            // If sink is not attached, we need to validate decoded frames here
            consumerThread.join();
        }

        // Keep decoder running while sink consumes output frames.
        // Session close/cleanup is handled explicitly by the caller.
        if (!mIsSinkAttached)
        {
            teardownDecoder(false);
        }
    }
    else
    {
        //TODO: Not used currently, update implementation if required in future
    }
}

void videoDecoderTest::setTestConfiguration(const videoDecoderTestConfiguration_t& config)
{
    mTestConfiguration = config;
}

void videoDecoderTest::closeDecoderSession()
{
    releaseDecoderSession(true);
}

void videoDecoderTest::getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped)
{
    optional<PropertyValue> propertyValue;

    framesDecoded = decodeErrors = framesDropped = -1;

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DECODED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_DECODE_ERRORS, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DROPPED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }
}

void videoDecoderTest::setupVideoDecoder()
{
    if (mVideoDecoderController.get() == nullptr)
    {
        mBufferListener = new AVBufferSpaceListener();

        // Create Video Pool
        AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, mBufferListener, mVideoPool);

        // Open Decoder — use openWithResolution() when hint dimensions are set in the test config
        if (mTestConfiguration.openWithResolutionWidth != -1 && mTestConfiguration.openWithResolutionHeight != -1)
        {
            UT_LOG_INFO("Opening decoder via openWithResolution(%dx%d)",
                        mTestConfiguration.openWithResolutionWidth, mTestConfiguration.openWithResolutionHeight);
            VIDEODECODER_OPEN_WITH_RESOLUTION_SUCCESS(
                mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener,
                mTestConfiguration.openWithResolutionWidth, mTestConfiguration.openWithResolutionHeight,
                mVideoDecoderController, true);
        }
        else
        {
            VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode,
                                      mControlListener, mVideoDecoderController, true);
        }
    }
    else
    {
        UT_LOG_INFO("Reusing open decoder for stream %s with codec %s",
                    mTestConfiguration.streamName.c_str(),
                    videodecoder::toString(mTestConfiguration.codec).c_str());
    }

    if(mVideoDecoderController.get() == nullptr)
    {
        UT_LOG_ERROR("VideoDecoderController is null after open");
        UT_ASSERT_TRUE(false) << "Failed to get VideoDecoderController after opening decoder";
        return;
    }

    if (mTunnelMode)
    {
        std::vector<std::pair<videodecoder::Property, int32_t>> startedIntProps = {
            {videodecoder::Property::OPERATIONAL_MODE, static_cast<int32_t>(OperationalMode::TUNNELLED)}
        };
        UT_LOG_DEBUG("Set properties in TUNNELLED Mode");
        VIDEODECODER_SETPROPERTY_LIST_SUCCESS(mVideoDecoderController, startedIntProps, intValue);
    }

    {
        videodecoder::VideoDecoderStreamConfig streamConfig;

        if (mTestConfiguration.codedWidth > 0 && mTestConfiguration.codedHeight > 0)
        {
            videodecoder::Resolution res;
            res.width  = mTestConfiguration.codedWidth;
            res.height = mTestConfiguration.codedHeight;
            streamConfig.resolution = res;
        }
        if (mTestConfiguration.frameRateDenominator > 0)
        {
            videodecoder::Fraction fr;
            fr.numerator   = mTestConfiguration.frameRateNumerator;
            fr.denominator = mTestConfiguration.frameRateDenominator;
            streamConfig.frameRate = fr;
        }
        Colorimetry unknownColorimetry{};
        if (mTestConfiguration.colorimetry != unknownColorimetry)
        {
            streamConfig.colorimetry = mTestConfiguration.colorimetry;
        }
        if (mTestConfiguration.masteringDisplayInfo.has_value())
        {
            streamConfig.masteringDisplayInfo = mTestConfiguration.masteringDisplayInfo;
        }
        if (mTestConfiguration.contentLightLevel.has_value())
        {
            streamConfig.contentLightLevel = mTestConfiguration.contentLightLevel;
        }
        UT_LOG_INFO("value of the dynamicRange %d", static_cast<int>(mTestConfiguration.dynamicRange));
        if (mTestConfiguration.dynamicRange == videodecoder::DynamicRange::DOLBY_VISION)
        {
            videodecoder::DolbyVisionLayerFlags dvFlags;
            dvFlags.blPresent = mTestConfiguration.blPresent;
            dvFlags.elPresent = mTestConfiguration.elPresent;
            streamConfig.dolbyVisionLayerFlags = dvFlags;
        }
        VIDEODECODER_SETSTREAMCONFIG_SUCCESS(mVideoDecoderController, streamConfig);
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

        if (videoBufferHandle == 0)
        {
            UT_LOG_ERROR("Alloc failed for frame %zu, skipping", i);
            continue;
        }

        // Write Frame Data
        void* mapBuffer = nullptr;
        uint32_t mapSize = 0;
        if (mSecureDecode)
        {
            UT_LOG_INFO("Calling writeSecureHandle(handle:0x%ld, size=%u, mAVbufferHelper:%p)",
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
            UT_LOG_INFO("Calling mapHandle(handle:0x%ld, mAVbufferHelper:%p)", videoBufferHandle, mAVbufferHelper);

            mapBuffer = mAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
            if (mapBuffer && mapSize >= frameSize)
            {
                UT_LOG_DEBUG("Copying frame data to mapped buffer, size=%u", frameSize);
                memcpy(mapBuffer, mInputBuffer.data() + frameOffset, frameSize);
                mAVbufferHelper->unmapHandle(videoBufferHandle);
            }
            else
            {
                UT_LOG_ERROR("Failed to map buffer or insufficient size (mapBuffer:%p, mapSize:%u, required:%u)",
                            mapBuffer, mapSize, frameSize);
                if (mapBuffer)
                {
                    mAVbufferHelper->unmapHandle(videoBufferHandle);
                }
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

        videodecoder::InputBufferMetadata metadata = {};
        metadata.nsPresentationTime = framePTS;
        metadata.discontinuity = false;
        const bool isEosControlPoint = (mTestConfiguration.eosPts == framePTS);

        VIDEODECODER_DECODEBUFFER_WITH_METADATA_SUCCESS(mVideoDecoderController, videoBufferHandle, metadata);

        const bool isFinalFrame = (i == (mFrameData.size() - 1));
        if (isEosControlPoint || isFinalFrame)
        {
            UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
            VIDEODECODER_SIGNAL_EOS_SUCCESS(mVideoDecoderController);
        }

        if (isEosControlPoint)
        {
            UT_LOG_INFO("Verifying post-EOS decode reject at PTS:%lld", framePTS);
            VIDEODECODER_DECODEBUFFER_WITH_METADATA_FAILURE(mVideoDecoderController, framePTS, videoBufferHandle,
                                                            false, false, Status::EX_NONE);
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
            VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(mTestConfiguration.decoderDelay));

        // EOS is sent via signalEndOfStream().
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
    int32_t eosCallbacksReceived = 0;
    int32_t discontinuityFramesReceived = 0;

    // Per contract, EOS fires exactly once per session (twice when eosPts triggers a mid-stream stop+restart).
    // Compute this before the loop so we can break as soon as all EOS signals are received.
    int32_t expectedEOSFrames = (mTestConfiguration.eosPts != -1) ? 2 : 1;

    while (mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(5000)))
    {
        UT_LOG_DEBUG("Retrieved remaining output frame: PTS=%lld, handle=0x%llx, isMetaAvailable=%d",
                decodedFrame.framePTS, decodedFrame.frameAVBufferHandle, decodedFrame.isMetaAvailable);

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
                    mInputFrame.erase(inputIt);
                }
            }

            if(decodedFrame.frameAVBufferHandle != -1)
            {
                //TODO: Need to validate buffer content as well. Currently just checking if buffer handle is valid and freeing it.
                AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameAVBufferHandle, isBufferValid);
                if (isBufferValid)
                {
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameAVBufferHandle);
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

            // Validate colorimetry — use override values for frames at or after metadataOverridePts
            const bool afterOverride = (mTestConfiguration.metadataOverridePts != -1 &&
                                        decodedFrame.framePTS >= mTestConfiguration.metadataOverridePts);
            if (afterOverride && mTestConfiguration.overrideColorimetry.has_value())
            {
                UT_ASSERT_EQUAL(decodedFrame.metadata->colorimetry, mTestConfiguration.overrideColorimetry.value())
                    << "Colorimetry override mismatch after metadataOverridePts="
                    << mTestConfiguration.metadataOverridePts;
            }
            else
            {
                UT_ASSERT_EQUAL(decodedFrame.metadata->colorimetry, mTestConfiguration.colorimetry)
                    << "Colorimetry mismatch: expected=" << mTestConfiguration.colorimetry.toString()
                    << ", got=" << decodedFrame.metadata->colorimetry.toString()
                    << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
            }

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

            if(decodedFrame.metadata->masteringDisplayInfo.has_value())
            {
                const auto& expectedMdi = (afterOverride && mTestConfiguration.overrideMasteringDisplayInfo.has_value())
                    ? mTestConfiguration.overrideMasteringDisplayInfo.value()
                    : mTestConfiguration.masteringDisplayInfo.value();
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->redPrimaryX,   expectedMdi.redPrimaryX);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->redPrimaryY,   expectedMdi.redPrimaryY);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->greenPrimaryX, expectedMdi.greenPrimaryX);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->greenPrimaryY, expectedMdi.greenPrimaryY);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->bluePrimaryX,  expectedMdi.bluePrimaryX);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->bluePrimaryY,  expectedMdi.bluePrimaryY);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->whitePointX,   expectedMdi.whitePointX);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->whitePointY,   expectedMdi.whitePointY);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->maxLuminance,  expectedMdi.maxLuminance);
                UT_ASSERT_EQUAL(decodedFrame.metadata->masteringDisplayInfo->minLuminance,  expectedMdi.minLuminance);
            }

            {
                const auto& expectedCll = (afterOverride && mTestConfiguration.overrideContentLightLevel.has_value())
                    ? mTestConfiguration.overrideContentLightLevel
                    : mTestConfiguration.contentLightLevel;
                UT_ASSERT_EQUAL(decodedFrame.metadata->contentLightLevel, expectedCll)
                    << "ContentLightLevel mismatch for Codec "
                    << videodecoder::toString(mTestConfiguration.codec).c_str();
            }

            if (afterOverride && mTestConfiguration.overridePixelAspectRatio.has_value())
            {
                const auto& expectedPar = mTestConfiguration.overridePixelAspectRatio.value();
                UT_ASSERT_EQUAL(decodedFrame.metadata->parX, expectedPar.numerator)
                    << "PAR numerator mismatch after metadataOverridePts="
                    << mTestConfiguration.metadataOverridePts
                    << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
                UT_ASSERT_EQUAL(decodedFrame.metadata->parY, expectedPar.denominator)
                    << "PAR denominator mismatch after metadataOverridePts="
                    << mTestConfiguration.metadataOverridePts
                    << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
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

    int32_t expectedFrames = mTestConfiguration.no_of_frames - mTestConfiguration.decodeErrors;

    if((mTestConfiguration.eosPts != -1))
    {
        // expectedEOSFrames already set to 2 above.
    }
    if((mTestConfiguration.discontinuityPts != -1) || (mTestConfiguration.flushPts !=-1))
    {
        UT_ASSERT_TRUE((discontinuityFramesReceived > 0 && discontinuityFramesReceived < expectedFrames));
    }

    UT_ASSERT_TRUE(mControlListener->waitForEndOfStream(expectedEOSFrames, std::chrono::milliseconds(5000)));
    eosCallbacksReceived = mControlListener->getEndOfStreamCount(false);
    UT_ASSERT_EQUAL(eosCallbacksReceived, expectedEOSFrames);

    totalOnErrorsReceived = mEventListener->getErrorCount();

    // Get final metrics
    getDecoderMetrics(metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped);
    UT_LOG_DEBUG("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec, metricsFramesDecoded, metricsDecodeErrors, metricsFramesDropped);

    if((mTestConfiguration.flushResetPts == -1) && (mTestConfiguration.flushPts==-1))
    {
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
    else
    {
        if(metricsFramesDecoded != -1)
        {
            UT_ASSERT_TRUE(metricsFramesDecoded <= expectedFrames);
        }
        UT_ASSERT_TRUE(totalOnFramesDecoded <= expectedFrames);

        if(metricsDecodeErrors != -1)
        {
            UT_ASSERT_TRUE(metricsDecodeErrors <= mTestConfiguration.decodeErrors);
        }
        UT_ASSERT_TRUE(totalOnErrorsReceived <= mTestConfiguration.decodeErrors);

        if(metricsFramesDropped != -1)
        {
            UT_ASSERT_TRUE(metricsFramesDropped <= mTestConfiguration.framesDropped);
        }
    }

    return true;
}

bool videoDecoderTest::runBackPressureTest()
{
    static constexpr uint32_t kInputSizeBytes = 4096;
    static constexpr int kMaxProbeSubmissions = 512;

    mControlListener->resetQueues();
    mControlListener->resetDecodeBufferAvailableCount();

    setupVideoDecoder();

    bool backPressureObserved = false;
    bool callbackObserved = false;
    int64_t pts = 1000000;

    auto freeIfRejected = [&](int64_t bufferHandle, const Status& status, bool decodeAccepted)
    {
        if (!decodeAccepted || !status.isOk() || status.exceptionCode() != Status::EX_NONE)
        {
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, bufferHandle);
        }
    };

    for (int i = 0; i < kMaxProbeSubmissions; ++i)
    {
        int64_t videoBufferHandle = 0;
        AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mVideoPool, kInputSizeBytes, videoBufferHandle);

        void* mapBuffer = nullptr;
        uint32_t mapSize = 0;
        mapBuffer = mAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
        if (mapBuffer && mapSize >= kInputSizeBytes)
        {
            memset(mapBuffer, 0, kInputSizeBytes);
            mAVbufferHelper->unmapHandle(videoBufferHandle);
        }
        else
        {
            if (mapBuffer) mAVbufferHelper->unmapHandle(videoBufferHandle);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
            UT_ASSERT_TRUE(false) << "Failed to map input buffer for back-pressure test";
            break;
        }

        videodecoder::InputBufferMetadata metadata = {};
        metadata.nsPresentationTime = pts;
        metadata.discontinuity = false;

        bool decodeResult = false;
        Status status = mVideoDecoderController->decodeBufferWithMetadata(videoBufferHandle, metadata, &decodeResult);
        freeIfRejected(videoBufferHandle, status, decodeResult);
        UT_ASSERT_TRUE(status.isOk()) << "decodeBufferWithMetadata binder call failed while probing back-pressure";
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE);

        if (!decodeResult)
        {
            backPressureObserved = true;
            break;
        }

        ++pts;
    }

    if (backPressureObserved)
    {
        callbackObserved = mControlListener->waitForDecodeBufferAvailable(1, std::chrono::milliseconds(5000));
        UT_ASSERT_TRUE(callbackObserved) << "onDecodeBufferAvailable was not received after back-pressure";
    }

    teardownDecoder();

    if (backPressureObserved)
    {
        return true;
    }

    UT_LOG_WARNING("[DecoderID: %d] Back-pressure not observed within %d submissions; callback semantics not asserted on this decoder",
                   mDecoderId.value, kMaxProbeSubmissions);
    return false;
}

void videoDecoderTest:: teardownDecoder(bool closeDecoderSession)
{
    releaseDecoderSession(closeDecoderSession);
}

void videoDecoderTest::releaseDecoderSession(bool closeController)
{
    if (mVideoDecoderController.get() == nullptr)
    {
        return;
    }

    ::com::rdk::hal::videodecoder::State currentState = ::com::rdk::hal::videodecoder::State::UNKNOWN;
    Status getStateStatus = mDecoderHandle->getState(&currentState);
    if (getStateStatus.isOk() && getStateStatus.exceptionCode() == Status::EX_NONE)
    {
        if (currentState == ::com::rdk::hal::videodecoder::State::STARTED ||
            currentState == ::com::rdk::hal::videodecoder::State::STARTING ||
            currentState == ::com::rdk::hal::videodecoder::State::FLUSHING ||
            currentState == ::com::rdk::hal::videodecoder::State::STOPPING)
        {
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        }
    }
    else
    {
        UT_LOG_WARNING("Skipping stop during releaseDecoderSession: failed to query decoder state");
    }

    if (closeController)
    {
        VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mVideoPool);
        mVideoDecoderController = nullptr;
        mBufferListener = nullptr;
    }
}

/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK
