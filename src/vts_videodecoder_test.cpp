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
Status AVBufferSpaceListener::onSpaceAvailable() {
    return Status::ok();
}


// videoDecoderTest method implementations
void videoDecoderTest::run(bool concurrentDecodeConsume)
{
    UT_LOG_INFO("concurrentDecodeConsume %d ",concurrentDecodeConsume);
    mConcurrentDecodeConsume = concurrentDecodeConsume;
    mDecodingStream = true;
    if (concurrentDecodeConsume)
    {
        UT_LOG_INFO("Start decoderThreadFunc & consumerThreadFunc");
        mDecoderThread = thread(&videoDecoderTest::decoderThreadFunc, this);
        this_thread::sleep_for(chrono::milliseconds(100));
        //mConsumerThread = thread(&videoDecoderTest::consumerThreadFunc, this);

        mDecoderThread.join();
        //mConsumerThread.join();
    }
    else
    {
        decodeAndConsumeVideoStreams();
    }
}

bool videoDecoderTest::getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped)
{
    optional<PropertyValue> propertyValue;

    framesDecoded = decodeErrors = framesDropped = -1;

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DECODED, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value()) {
        return false;
    }
    framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_DECODE_ERRORS, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value()) {
        return false;
    }
    decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

    VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, videodecoder::Property::METRIC_FRAMES_DROPPED, propertyValue);
    if (!propertyValue.has_value() || !propertyValue->value.has_value()) {
        return false;
    }
    framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

    return true;
}

std::vector<std::tuple<uint64_t, uint32_t, int64_t>> videoDecoderTest::parseFrames(const std::vector<uint8_t>& inputBuffer, uint64_t fileSize, com::rdk::hal::videodecoder::Codec codec) {
    std::vector<std::tuple<uint64_t, uint32_t, int64_t>> frames; // {offset, size, pts}

    if (mTestConfiguration.frameDetails.empty()) {
        UT_LOG_ERROR("frameDetails is empty");
        return frames;
    }

    switch (codec) {
        case com::rdk::hal::videodecoder::Codec::H264_AVC:
        case com::rdk::hal::videodecoder::Codec::H265_HEVC: {
            uint64_t pos = 0;
            size_t frameIndex = 0;
            while (pos < fileSize - 4) {
                bool isStartCode = false;
                uint32_t startCodeLen = 0;
                if (pos < fileSize - 3 && inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 && inputBuffer[pos + 2] == 1) {
                    isStartCode = true;
                    startCodeLen = 3;
                } else if (pos < fileSize - 4 && inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 &&
                           inputBuffer[pos + 2] == 0 && inputBuffer[pos + 3] == 1) {
                    isStartCode = true;
                    startCodeLen = 4;
                }

                if (isStartCode) {
                    uint64_t start = pos;
                    pos += startCodeLen;
                    uint64_t nextPos = pos;
                    while (nextPos < fileSize - 3) {
                        if ((inputBuffer[nextPos] == 0 && inputBuffer[nextPos + 1] == 0 && inputBuffer[nextPos + 2] == 1) ||
                            (nextPos < fileSize - 4 && inputBuffer[nextPos] == 0 && inputBuffer[nextPos + 1] == 0 &&
                             inputBuffer[nextPos + 2] == 0 && inputBuffer[nextPos + 3] == 1)) {
                            break;
                        }
                        nextPos++;
                    }
                    uint32_t frameSize = nextPos - start;
                    if (frameSize > startCodeLen) {
                        int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                                      std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                                      frameIndex * 33333333; // Default 30fps
                        frames.emplace_back(start, frameSize, pts);
                        UT_LOG_INFO("Frame detected: offset=%llu, size=%u, pts=%lld", start, frameSize, pts);
                    }
                    pos = nextPos;
                    frameIndex++;
                } else {
                    pos++;
                }
            }
            break;
        }
        case com::rdk::hal::videodecoder::Codec::MPEG2_VIDEO: {
            uint64_t pos = 0;
            size_t frameIndex = 0;
            while (pos < fileSize - 4) {
                if (inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 && inputBuffer[pos + 2] == 1 &&
                    inputBuffer[pos + 3] >= 0xE0 && inputBuffer[pos + 3] <= 0xEF) { // Video stream ID
                    uint64_t start = pos;
                    pos += 4;
                    if (pos + 2 >= fileSize) break;
                    uint32_t pesLength = (inputBuffer[pos] << 8) | inputBuffer[pos + 1];
                    pos += 2;
                    uint32_t frameSize = pesLength + 6; // Include start code and length
                    if (start + frameSize > fileSize) break;
                    int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                                  std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                                  frameIndex * 33333333; // Default 30fps
                    frames.emplace_back(start, frameSize, pts);
                    UT_LOG_INFO("MPEG PES packet detected: offset=%llu, size=%u, pts=%lld", start, frameSize, pts);
                    pos = start + frameSize;
                    frameIndex++;
                } else {
                    pos++;
                }
            }
            break;
        }
        case com::rdk::hal::videodecoder::Codec::VP9: {
            uint64_t pos = 32; // Skip IVF header (fixed 32 bytes)
            size_t frameIndex = 0;
            while (pos + 12 <= fileSize) {
                uint32_t frameSize = inputBuffer[pos] |
                                     (inputBuffer[pos + 1] << 8) |
                                     (inputBuffer[pos + 2] << 16) |
                                     (inputBuffer[pos + 3] << 24);
                uint64_t timestamp = static_cast<uint64_t>(inputBuffer[pos + 4]) |
                                     (static_cast<uint64_t>(inputBuffer[pos + 5]) << 8) |
                                     (static_cast<uint64_t>(inputBuffer[pos + 6]) << 16) |
                                     (static_cast<uint64_t>(inputBuffer[pos + 7]) << 24) |
                                     (static_cast<uint64_t>(inputBuffer[pos + 8]) << 32) |
                                     (static_cast<uint64_t>(inputBuffer[pos + 9]) << 40) |
                                     (static_cast<uint64_t>(inputBuffer[pos + 10]) << 48) |
                                     (static_cast<uint64_t>(inputBuffer[pos + 11]) << 56);

                uint64_t frameStart = pos + 12;
                if (frameStart + frameSize > fileSize) break;

                int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                              std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                              static_cast<int64_t>(timestamp); // Use IVF timestamp
                frames.emplace_back(frameStart, frameSize, pts);
                UT_LOG_INFO("VP9 IVF frame: offset=%llu, size=%u, pts=%lld", frameStart, frameSize, pts);

                pos = frameStart + frameSize;
                frameIndex++;
            }
            break;
        }
        case com::rdk::hal::videodecoder::Codec::AV1: {
            uint64_t pos = 0;
            size_t frameIndex = 0;
            while (pos < fileSize) {
                if (pos + 1 >= fileSize) break;
                uint8_t obuHeader = inputBuffer[pos];
                uint32_t obuType = (obuHeader >> 3) & 0x1F;
                bool hasSize = (obuHeader & 0x02) == 0;
                uint64_t start = pos;
                pos++;
                uint32_t obuSize = 0;
                if (hasSize) {
                    for (int i = 0; i < 8 && pos < fileSize; i++) {
                        uint8_t byte = inputBuffer[pos++];
                        obuSize |= (byte & 0x7F) << (i * 7);
                        if (!(byte & 0x80)) break;
                    }
                }
                if (obuType == 1 || obuType == 3 || obuType == 6) { // Frame or Temporal Unit OBUs
                    uint32_t frameSize = obuSize + (pos - start);
                    if (start + frameSize > fileSize) break;
                    int64_t pts = (frameIndex < mTestConfiguration.frameDetails.size()) ?
                                  std::get<1>(mTestConfiguration.frameDetails[frameIndex]) :
                                  frameIndex * 33333333; // Default 30fps
                    frames.emplace_back(start, frameSize, pts);
                    UT_LOG_INFO("AV1 OBU detected: offset=%llu, size=%u, pts=%lld", start, frameSize, pts);
                    pos += obuSize;
                    frameIndex++;
                } else {
                    pos += obuSize;
                }
            }
            break;
        }
        default:
            UT_LOG_ERROR("Unsupported codec: %d", static_cast<int>(codec));
            return frames;
    }

    if (!frames.empty() && frames.size() != mTestConfiguration.frameDetails.size()) {
        UT_LOG_WARNING("Frame count mismatch: parsed=%zu, expected=%zu", frames.size(), mTestConfiguration.frameDetails.size());
    }
    return frames;
}

void videoDecoderTest::decoderThreadFunc()
{
        sp<IVideoDecoderController> videoDecoderController;
        int64_t videoBufferHandle = 0;
        Pool videoPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();
        mDecodingStream = true;
        bool isBufferValid = false;

        DecodedVideoFrame_t decodedFrame;
        // Initialize PTS tracking
        std::set<int64_t> mReceivedPts(mTestConfiguration.expectedPts.begin(), mTestConfiguration.expectedPts.end());

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc Start", getpid(), syscall(SYS_gettid));

        // Validate mAVbufferHelper
        if (!mAVbufferHelper) {
            UT_LOG_ERROR("mAVbufferHelper is null");
            mDecodingStream = false;
            return;
        }
        UT_LOG_INFO("mAVbufferHelper initialized: %p, secureDecode: %d", mAVbufferHelper, mSecureDecode);

        // Create Video Pool
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Calling createVideoPool(secureHeap:%d, videoDecoderId:%d, listener:%p)",
                        mSecureDecode, mDecoderId, bufferListener.get());
            AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, videoPool);
            UT_LOG_INFO("Created video pool with handle:%lld", videoPool.handle);
        }

        // Validate and read input file
        if (!std::filesystem::exists(mTestConfiguration.input)) {
            UT_LOG_ERROR("Input file does not exist: %s", mTestConfiguration.input.c_str());
            mDecodingStream = false;
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
            return;
        }
        std::ifstream inputFile(mTestConfiguration.input, std::ios::binary);
        if (!inputFile.is_open()) {
            UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
            mDecodingStream = false;
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
            return;
        }

        inputFile.seekg(0, std::ios::end);
        auto fileSizePos = inputFile.tellg();
        if (fileSizePos < 0) {
            UT_LOG_ERROR("Failed to get file size for %s: tellg returned %lld",
                        mTestConfiguration.input.c_str(), static_cast<int64_t>(fileSizePos));
            mDecodingStream = false;
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
            return;
        }
        uint64_t fileSize = static_cast<uint64_t>(fileSizePos);
        if (fileSize == 0) {
            UT_LOG_ERROR("Input file is empty: %s", mTestConfiguration.input.c_str());
            mDecodingStream = false;
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
            return;
        }
        if (fileSize > std::numeric_limits<uint32_t>::max()) {
            UT_LOG_ERROR("File size too large: %llu bytes, max allowed: %u bytes",
                        fileSize, std::numeric_limits<uint32_t>::max());
            mDecodingStream = false;
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
            return;
        }
        UT_LOG_INFO("Input file size: %llu bytes", fileSize);

        std::vector<uint8_t> inputBuffer(fileSize);
        try {
            inputFile.seekg(0, std::ios::beg);
            inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), fileSize);
            if (!inputFile) {
                UT_LOG_ERROR("Failed to read file data: only %zu bytes read", inputFile.gcount());
                mDecodingStream = false;
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
                AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
                return;
            }
            uint64_t actualSize = static_cast<uint64_t>(inputFile.gcount());
            if (actualSize != fileSize) {
                UT_LOG_WARNING("Read %llu bytes, expected %llu bytes", actualSize, fileSize);
                fileSize = actualSize;
            }
        } catch (const std::exception& e) {
            UT_LOG_ERROR("Exception reading file: %s", e.what());
            mDecodingStream = false;
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
            return;
        }

        // Determine frame data based on codec
        std::vector<std::tuple<uint64_t, uint32_t, int64_t, bool>> frameData;
        if (mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H264_AVC ||
            mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H265_HEVC) {
            if (mTestConfiguration.frameDetails.empty()) {
                UT_LOG_ERROR("frameDetails is empty for H.264/H.265");
                mDecodingStream = false;
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
                AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
                return;
            }
            uint64_t offset = 0;
            for (const auto& [size, pts, isErrorFrame] : mTestConfiguration.frameDetails) {
                if (offset + size > fileSize) {
                    UT_LOG_ERROR("Frame size at offset %llu exceeds file size %llu", offset + size, fileSize);
                    mDecodingStream = false;
                    std::lock_guard<std::mutex> lock(mDecodeMutex);
                    UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
                    AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
                    return;
                }
                frameData.emplace_back(offset, size, pts, isErrorFrame);
                UT_LOG_INFO("Codec:%d Frame from frameDetails: offset=%llu, size=%u, pts=%lld, errorFrame=%d",
                            mTestConfiguration.codec, offset, size, pts, isErrorFrame);
                offset += size;
            }
        } else {
            auto parsedFrames = parseFrames(inputBuffer, fileSize, mTestConfiguration.codec);
            if (parsedFrames.empty()) {
                UT_LOG_ERROR("No frames parsed for codec: %d", static_cast<int>(mTestConfiguration.codec));
                mDecodingStream = false;
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
                AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
                return;
            }
            for (size_t i = 0; i < parsedFrames.size(); ++i) {
                auto [offset, size, pts] = parsedFrames[i];
                bool isErrorFrame = (i < mTestConfiguration.frameDetails.size()) ?
                                    std::get<2>(mTestConfiguration.frameDetails[i]) : false;
                frameData.emplace_back(offset, size, pts, isErrorFrame);
                UT_LOG_INFO("Parsed frame: Codec:%d offset=%llu, size=%u, pts=%lld, errorFrame=%d",
                            mTestConfiguration.codec, offset, size, pts, isErrorFrame);
            }
        }

        // Open Decoder
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Calling open(codec:%d, secure:%d, listener:%p)",
                        mTestConfiguration.codec, mSecureDecode, mControlListener.get());
            VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode,
                                    mControlListener, videoDecoderController, true);
            UT_LOG_INFO("Decoder opened, controller:%p", videoDecoderController.get());

            if (mTunnelMode) {
                std::vector<std::pair<videodecoder::Property, int32_t>> startedIntProps = {
                    {videodecoder::Property::OPERATIONAL_MODE, static_cast<int32_t>(OperationalMode::TUNNELLED)}
                };
                UT_LOG_INFO("Set properties in TUNNELLED Mode");
                VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, startedIntProps, intValue);
            }
            std::optional<PropertyValue> aidl_return;
            VIDEODECODER_GETPROPERTY(mDecoderHandle, videodecoder::Property::OPERATIONAL_MODE, aidl_return);
        }

        // Start Decoder
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
            UT_LOG_INFO("Decoder started");
        }

        // Get initial metrics
        int32_t framesDecoded, decodeErrors, framesDropped;
        getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("Initial Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec, framesDecoded, decodeErrors, framesDropped);

        // Process Frames
        for (size_t i = 0; i < frameData.size(); ++i) {
            auto [frameOffset, frameSize, framePTS, isErrorFrame] = frameData[i];

            UT_LOG_INFO("Processing frame %zu: offset=%llu, size=%u, PTS=%lldns, errorFrame=%d",
                        i, frameOffset, frameSize, framePTS, isErrorFrame);

            // Allocate Buffer
            {
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                UT_LOG_INFO("Calling alloc(poolHandle:%lld, size=%u)", videoPool.handle, frameSize);
                videoBufferHandle = 0;
                AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, videoPool, frameSize, videoBufferHandle);
                UT_LOG_INFO("Allocated buffer handle:0x%llx", videoBufferHandle);
            }

            // Write Frame Data
            void* mapBuffer = nullptr;
            uint32_t mapSize = 0;
            if (mSecureDecode) {
                std::lock_guard<std::mutex> lock(mAVbufferHelperLock);
                UT_LOG_INFO("Calling writeSecureHandle(handle:0x%llx, size=%u, mAVbufferHelper:%p)",
                            videoBufferHandle, frameSize, mAVbufferHelper);
                if (!mAVbufferHelper || !mAVbufferHelper->writeSecureHandle(videoBufferHandle, inputBuffer.data() + frameOffset, frameSize)) {
                    UT_LOG_ERROR("Failed to copy frame data to secure buffer");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    continue;
                }
            } else {
                std::lock_guard<std::mutex> lock(mAVbufferHelperLock);
                UT_LOG_INFO("Calling mapHandle(handle:0x%llx, mAVbufferHelper:%p)", videoBufferHandle, mAVbufferHelper);
                if (!mAVbufferHelper) {
                    UT_LOG_ERROR("mAVbufferHelper is null before mapHandle");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    continue;
                }
                mapBuffer = mAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
                if (mapBuffer && mapSize >= frameSize) {
                    UT_LOG_INFO("Copying frame data to mapped buffer, size=%u", frameSize);
                    memcpy(mapBuffer, inputBuffer.data() + frameOffset, frameSize);
                } else {
                    UT_LOG_ERROR("Failed to map buffer or insufficient size (mapBuffer:%p, mapSize:%u, required:%u)",
                                mapBuffer, mapSize, frameSize);
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    continue;
                }
            }

            // Decoder Operations
            {
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                if (mTestConfiguration.stopPts == framePTS) {
                    UT_LOG_INFO("Stopping decoder at PTS:%lld", framePTS);
                    VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
                    VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
                }
                if (mTestConfiguration.discontinuityPts == framePTS) {
                    UT_LOG_INFO("Signaling discontinuity at PTS:%lld", framePTS);
                    VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, videoDecoderController, true);
                }
                if (mTestConfiguration.flushPts == framePTS) {
                    UT_LOG_INFO("Flushing decoder at PTS:%lld", framePTS);
                    VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, videoDecoderController, false, true);
                }
                if (mTestConfiguration.flushResetPts == framePTS) {
                    UT_LOG_INFO("Flush reset at PTS:%lld", framePTS);
                    VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, videoDecoderController, true, true);
                    VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
                }

                InputVideoFrame_t inputVideo;
                inputVideo.sendTime = std::chrono::steady_clock::now();
                inputVideo.framePTS = framePTS;
                inputVideo.rms_threshold = mTestConfiguration.rms_diff_threshold;
                inputVideo.isErrorFrame = isErrorFrame;
                UT_LOG_INFO("Adding frame to mInputFrame, PTS:%lld, errorFrame=%d", framePTS, isErrorFrame);
                mInputFrame.push_back(inputVideo);
            }

            auto decodeStart = std::chrono::steady_clock::now();

            // Decode Frame
            {
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                if (!videoDecoderController) {
                    UT_LOG_ERROR("videoDecoderController is null");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    if (!mSecureDecode && mapBuffer) {
                        UT_LOG_INFO("Unmapping buffer handle:0x%llx", videoBufferHandle);
                        mAVbufferHelper->unmapHandle(videoBufferHandle);
                    }
                    continue;
                }
                UT_LOG_INFO("Calling decodeBuffer(PTS:%lld, bufferHandle:0x%llx)", framePTS, videoBufferHandle);
                if (isErrorFrame) {
                    VIDEODECODER_DECODEBUFFER_FAILURE(videoDecoderController, framePTS, videoBufferHandle, Status::EX_NONE);
                } else {
                    VIDEODECODER_DECODEBUFFER_SUCCESS(videoDecoderController, framePTS, videoBufferHandle);
                }
                getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
                UT_LOG_INFO("Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                            mTestConfiguration.codec, framesDecoded, decodeErrors, framesDropped);
            }

            if (mTestConfiguration.eosPts == framePTS) {
                   UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
                    VIDEODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, videoDecoderController, true);
                    VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
                    VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
                }

            // Check decode time
            auto decodeEnd = std::chrono::steady_clock::now();
            auto decodeTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(decodeEnd - decodeStart).count();
            if (decodeTimeNs > mTestConfiguration.decode_time_threshold) {
                UT_LOG_WARNING("Frame %zu decode time %lld ns exceeds threshold %lld ns",
                            i, decodeTimeNs, mTestConfiguration.decode_time_threshold);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(mTestConfiguration.decoderDelay));

            // Free Input Buffer
            {
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                bool isValidBuffer = false;
                UT_LOG_INFO("Calling isValid(IN:bufferHandle:[0X%016"PRIx64"], OUT:_aidl_return[])", videoBufferHandle);
                Status status = mAVBufferManager->isValid(videoBufferHandle, &isValidBuffer);
                UT_LOG_INFO("Result isValid(IN:bufferHandle:[0X%016"PRIx64"], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    videoBufferHandle, UT_Control_GetMapString(bool_mapTable, isValidBuffer), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
                if(isValidBuffer) {
                    UT_LOG_ERROR("Encoded buffer not released by Decoder [handle:0x%llx]", videoBufferHandle);
                }
            }
        }

            // Retrieve any remaining output frames
            while (mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(100)))
            {
                UT_LOG_INFO("Retrieved remaining output frame: PTS=%lld, handle=0x%llx, isMetaAvailable=%d",
                        decodedFrame.framePTS, decodedFrame.frameBufferHandle, decodedFrame.isMetaAvailable);
                // Validate PTS
                {
                    auto it = mReceivedPts.find(decodedFrame.framePTS);
                    if (it == mReceivedPts.end()) {
                        UT_LOG_ERROR("Unexpected PTS received: %lld", decodedFrame.framePTS);
                    } else {
                        UT_LOG_INFO("Validated PTS: %lld", decodedFrame.framePTS);
                        mReceivedPts.erase(it); // Remove to track received PTS
                    }
                }
                if (decodedFrame.metadata) {
                        UT_LOG_INFO("Validating metadata for PTS=%lld", decodedFrame.framePTS);
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

                        // Validate flags
                        if(mTestConfiguration.eosPts == decodedFrame.framePTS)
                        {
                            UT_ASSERT_EQUAL(decodedFrame.metadata->endOfStream, mTestConfiguration.endOfStream)
                                << "endOfStream mismatch: expected=" << mTestConfiguration.endOfStream
                                << ", actual=" << decodedFrame.metadata->endOfStream
                                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
                        }

                        if(mTestConfiguration.discontinuityPts == decodedFrame.framePTS)
                        {
                            UT_ASSERT_EQUAL(decodedFrame.metadata->discontinuity, mTestConfiguration.discontinuity)
                                << "discontinuity mismatch: expected=" << mTestConfiguration.discontinuity
                                << ", actual=" << decodedFrame.metadata->discontinuity
                                << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
                        }

                        UT_ASSERT_EQUAL(decodedFrame.metadata->lowLatency, mTestConfiguration.lowLatency)
                            << "lowLatency mismatch: expected=" << mTestConfiguration.lowLatency
                            << ", actual=" << decodedFrame.metadata->lowLatency
                            << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
                    } else {
                        UT_LOG_INFO("No metadata available for PTS=%lld", decodedFrame.framePTS);
                    }
            }

        // Get final metrics
        getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec, framesDecoded, decodeErrors, framesDropped);

        UT_ASSERT_EQUAL(framesDecoded, mTestConfiguration.no_of_frames - mTestConfiguration.decodeErrors)
                    << "Frames Decoded mismatch: expected=" << (mTestConfiguration.no_of_frames - mTestConfiguration.decodeErrors)
                    << ", actual=" << framesDecoded
                    << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
        UT_ASSERT_EQUAL(decodeErrors, mTestConfiguration.decodeErrors)
                        << "Frames Decoded mismatch: expected=" << mTestConfiguration.decodeErrors
                        << ", actual=" << decodeErrors
                        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();
        UT_ASSERT_EQUAL(framesDropped, mTestConfiguration.framesDropped)
                        << "Frames Decoded mismatch: expected=" << mTestConfiguration.framesDropped
                        << ", actual=" << framesDropped
                        << ", for Codec " << videodecoder::toString(mTestConfiguration.codec).c_str();

        // Stop Decoder
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Stopping decoder");
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
        }

        getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("After Stop Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec, framesDecoded, decodeErrors, framesDropped);

        // Close Decoder
        VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, videoDecoderController, true);

        // Destroy Video Pool
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            PoolMetrics poolMetrics;
            UT_LOG_INFO("Calling getPoolMetrics(IN:poolHandle:[%d], OUT:PoolMetrics[])", videoPool.handle);
            Status status = mAVBufferManager->getPoolMetrics(videoPool, &poolMetrics);
            UT_LOG_INFO("Result getPoolMetrics(IN:poolHandle:[%d], OUT:PoolMetrics[bytesUsed:%d, bytesTotal:%d]), status:[isOK:%s, exception:%s]", \
                videoPool.handle, poolMetrics.bytesUsed, poolMetrics.bytesTotal, \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
        }

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc End", getpid(), syscall(SYS_gettid));
        mDecodingStream = false;
}

void videoDecoderTest::consumerThreadFunc() {
    int32_t errorCount = 0;
    UT_LOG_INFO("[PID: %d, TID: %d] consumerThreadFunc Start", getpid(), syscall(SYS_gettid));

    while (mDecodingStream || !mInputFrame.empty()) {
        DecodedVideoFrame_t decodedFrame;
        InputVideoFrame_t inputFrame;

        if (!mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(50))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        bool found = false;
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex); // Note: Should be mDecodeMutex, not mMutex
            for (auto it = mInputFrame.begin(); it != mInputFrame.end(); ++it) {
                if (it->framePTS == decodedFrame.framePTS) { // Inconsistent: frameCount vs framePTS
                    inputFrame = std::move(*it);
                    mInputFrame.erase(it);
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            UT_LOG_ERROR("No input frame found for decoded frame PTS=%lld", decodedFrame.framePTS);
            continue;
        }

        if (inputFrame.isErrorFrame) {
            errorCount++;
            continue;
        }

        UT_ASSERT_EQUAL(decodedFrame.framePTS, inputFrame.framePTS)
            << "PTS mismatch: decoded=" << decodedFrame.framePTS << ", input=" << inputFrame.framePTS;

        if (decodedFrame.isMetaAvailable && decodedFrame.metadata) {
            UT_LOG_INFO("Validating metadata for PTS=%lld", decodedFrame.framePTS);
            UT_ASSERT_EQUAL(decodedFrame.metadata->codedWidth, mTestConfiguration.codedWidth)
                << "Width mismatch: expected=" << mTestConfiguration.codedWidth
                << ", actual=" << decodedFrame.metadata->codedWidth;
            UT_ASSERT_EQUAL(decodedFrame.metadata->codedHeight, mTestConfiguration.codedHeight)
                << "Height mismatch: expected=" << mTestConfiguration.codedHeight
                << ", actual=" << decodedFrame.metadata->codedHeight;
            UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateNumerator, mTestConfiguration.frameRateNumerator)
                << "Frame rate numerator mismatch: expected=" << mTestConfiguration.frameRateNumerator
                << ", actual=" << decodedFrame.metadata->frameRateNumerator;
            UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateDenominator, mTestConfiguration.frameRateDenominator)
                << "Frame rate denominator mismatch: expected=" << mTestConfiguration.frameRateDenominator
                << ", actual=" << decodedFrame.metadata->frameRateDenominator;
        } else {
            UT_LOG_WARNING("No metadata available for PTS=%lld", decodedFrame.framePTS);
        }

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            decodedFrame.receiveTime - inputFrame.sendTime).count();
        UT_LOG_INFO("Decode Time in msec [PTS: %lld]: %lld", decodedFrame.framePTS, duration / 1000);
    }

    if (errorCount > 0) {
        UT_LOG_INFO("No. of Error frames pushed: %d", errorCount);
        UT_LOG_INFO("No. of Error frames Callbacks: %d", mEventListener->getErrors().size());
        UT_ASSERT_EQUAL(mEventListener->getErrors().size(), errorCount);
    }

    UT_LOG_INFO("[PID: %d, TID: %d] consumerThreadFunc end", getpid(), syscall(SYS_gettid));
}

void videoDecoderTest::decodeAndConsumeVideoStreams() {
    sp<IVideoDecoderController> videoDecoderController;
    int64_t videoBufferHandle;
    Pool videoPool;
    sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

    UT_LOG_INFO("decodeAndConsumeVideoStreams start:");

    AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, videoPool);

    ifstream inputFile(mTestConfiguration.input, ios::binary);
    if (!inputFile) {
        UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
        return;
    }

    VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, videoDecoderController, true);

    VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);

    for (const auto& [frameSize, framePTS, errorFrame] : mTestConfiguration.frameDetails) {
        vector<uint8_t> inputBuffer;
        DecodedVideoFrame_t decodedFrame;

        AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, videoPool, frameSize, videoBufferHandle);

        inputBuffer.resize(frameSize);
        inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

        auto startTime = chrono::steady_clock::now();
        VIDEODECODER_DECODEBUFFER_SUCCESS(videoDecoderController, framePTS, videoBufferHandle);

        mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(100));

        if (!mTunnelMode && !decodedFrame.data.empty()) {
            UT_LOG_INFO("Validating frame data for PTS=%lld", decodedFrame.framePTS);
        }

        auto duration = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - startTime);
        UT_LOG_INFO("Decode Time [PTS: %lld]: %lld", framePTS, duration.count());

        bool isValidBuffer = false;
        UT_LOG_INFO("Calling isValid(IN:bufferHandle:[0X%016"PRIx64"], OUT:_aidl_return[])", videoBufferHandle);
        Status status = mAVBufferManager->isValid(videoBufferHandle, &isValidBuffer);
        UT_LOG_INFO("Result isValid(IN:bufferHandle:[0X%016"PRIx64"], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
            videoBufferHandle, UT_Control_GetMapString(bool_mapTable, isValidBuffer), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        if(isValidBuffer) {
            UT_LOG_ERROR("Encoded buffer not released by deecoder [handle:0x%llx]", videoBufferHandle);
        }
    }

    VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);

    VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, videoDecoderController, true);

    PoolMetrics poolMetrics;
    UT_LOG_INFO("Calling getPoolMetrics(IN:poolHandle:[%d], OUT:PoolMetrics[])", videoPool.handle); \
    Status status = mAVBufferManager->getPoolMetrics(videoPool, &poolMetrics); \
    UT_LOG_INFO("Result getPoolMetrics(IN:poolHandle:[%d], OUT:PoolMetrics[bytesUsed:%d, bytesTotal:%d]), status:[isOK:%s, exception:%s]", \
                videoPool.handle, poolMetrics.bytesUsed, poolMetrics.bytesTotal, \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \

    AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
}

/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK