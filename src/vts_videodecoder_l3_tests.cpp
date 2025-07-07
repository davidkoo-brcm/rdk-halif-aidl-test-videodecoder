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
 * @defgroup VIDEO_DECODER_HALTEST_L3 Video Decoder HAL Tests L3 File
 * @{
 * @parblock
 *
 * ### L3 Test Cases for Video Decoder HAL :
 *
 * This module includes Level 3 functional tests (success and failure scenarios).
 * This is to ensure that the API meets the operational requirements of the module across all vendors.
 *
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 *
 * Refer to UT specification documentation [videoDecoder_High-Level_TestSpecification.md]
 *
 * @endparblock
 */

/**
 * @file vts_videodecoder_l3_tests.cpp
 *
 */


#include "vts_videodecoder.h"
#include <fstream>
#include <vector>
#include <future>

using namespace std::chrono;
using namespace std;


/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L3_VideoDecoder, UT_TESTS_L3)


static char *gTestGroup = "L3";
static IAVBufferHelper* gAVbufferHelper;

#define AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(avBufferManager, secureHeap, videoDecoderId, listener, videoPool) \
    do { \
        UT_LOG_INFO("Calling createVideoPool(IN:secureHeap:[%d], IN:videoDecoderId:[%d], IN:listener:[%p], OUT:videoPool)", \
                    secureHeap, videoDecoderId.value, listener.get()); \
        Status status = avBufferManager->createVideoPool(secureHeap, videoDecoderId, listener, &videoPool); \
        UT_LOG_INFO("Result createVideoPool(IN:secureHeap:[%d], IN:videoDecoderId:[%d], IN:listener:[%p], OUT:videoPool.handle:[%d]), status:[isOK:%s, exception:%s]", \
                    secureHeap, videoDecoderId.value, listener.get(), videoPool.handle, \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_NOT_EQUAL(videoPool.handle, Pool::INVALID); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AVBUFFER_DESTROY_POOL_SUCCESS(avBufferManager, pool) \
    do { \
        bool _aidl_return = false; \
        Status status; \
        UT_LOG_INFO("Calling destroyPool(IN:poolHandle:[%d], OUT:_aidl_return:[])", pool); \
        status = avBufferManager->destroyPool(pool, &_aidl_return); \
        UT_LOG_INFO("Result destroyPool with (IN:poolHandle:[%d], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    pool.handle, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AVBUFFER_ALLOC_SUCCESS(avBufferManager, pool, size, bufferHandle) \
    do { \
        UT_LOG_INFO("Calling alloc(IN:poolHandle:[%d], IN:size:[%d], OUT:bufferHandle)", pool.handle, size); \
        Status status = avBufferManager->alloc(pool, size, &bufferHandle); \
        UT_LOG_INFO("Result alloc(IN:poolHandle:[%d], IN:size:[%d], OUT:bufferHandle:[0x%X]), status:[isOK:%s, exception:%s]", \
                    pool.handle, size, bufferHandle, UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_NOT_EQUAL(bufferHandle, IAVBuffer::INVALID_HANDLE); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AVBUFFER_FREE_SUCCESS(avBufferManager, bufferHandle) \
    do { \
        bool _aidl_return = false; \
        UT_LOG_INFO("Calling free(IN:bufferHandle:[0x%X], OUT:_aidl_return)", bufferHandle); \
        Status status = avBufferManager->free(bufferHandle, &_aidl_return); \
        UT_LOG_INFO("Result free(IN:bufferHandle:[0x%X], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    bufferHandle, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(_aidl_return); \
    } while (0)

#define AVBUFFER_GETMETRICS(avBufferManager, secureHeap, heapMetrics) \
    do { \
        UT_LOG_INFO("Calling getHeapMetrics(IN:secureHeap:[%s], OUT:heapMetrics[])", \
                    UT_Control_GetMapString(bool_mapTable, secureHeap)); \
        Status status = avBufferManager->getHeapMetrics(secureHeap, &heapMetrics); \
        UT_LOG_INFO("Result getHeapMetrics(IN:secureHeap:[%s], OUT:heapMetrics[secure:%s, bytesUsed:%d, bytesTotal:%d]), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, secureHeap), \
                    UT_Control_GetMapString(bool_mapTable, heapMetrics.secure), \
                    heapMetrics.bytesUsed, \
                    heapMetrics.bytesTotal, \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

class AVBufferSpaceListener : public BnAVBufferSpaceListener
{
public:
    AVBufferSpaceListener() {}
    virtual ~AVBufferSpaceListener() = default;

    Status onSpaceAvailable() override
    {
        return Status::ok();
    }
};

class DecoderTest
{
public:
    DecoderTest(IVideoDecoder::Id decoderId, sp<IVideoDecoder> decoderHandle,
                sp<VideoDecoderControllerListener> controlListener,
                sp<VideoDecoderEventListener> eventListener,
                sp<IAVBuffer> AVBufferManager,
                bool secureDecode, bool tunnelMode, testConfiguration_t config)
    {
        mDecoderId = decoderId;
        mDecoderHandle = decoderHandle;
        mControlListener = controlListener;
        mSecureDecode = secureDecode;
        mTunnelMode = tunnelMode;
        mEventListener = eventListener;
        mTestConfiguration = std::move(config);;
        mAVBufferManager = AVBufferManager;
        mDecodingStream = false;
    }

    void run(bool concurrentDecodeConsume)
    {
        UT_LOG_INFO("concurrentDecodeConsume %d ",concurrentDecodeConsume);
        mConcurrentDecodeConsume = concurrentDecodeConsume;
        mDecodingStream = true;
        if (concurrentDecodeConsume)
        {
            UT_LOG_INFO("Start decoderThreadFunc & consumerThreadFunc");
            mDecoderThread = thread(&DecoderTest::decoderThreadFunc, this);
            this_thread::sleep_for(chrono::milliseconds(100));
            //mConsumerThread = thread(&DecoderTest::consumerThreadFunc, this);

            mDecoderThread.join();
            //mConsumerThread.join();
        }
        else
        {
            decodeAndConsumeVideoStreams();
        }
    }

    bool getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped)
    {
        optional<PropertyValue> propertyValue;

        framesDecoded = decodeErrors = framesDropped = -1;

        VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, Property::METRIC_FRAMES_DECODED, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, Property::METRIC_DECODE_ERRORS, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        VIDEODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, Property::METRIC_FRAMES_DROPPED, propertyValue);
        if (!propertyValue.has_value() || !propertyValue->value.has_value())
        {
            return false;
        }
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();

        return true;
    }

    // Parse frames based on codec type
    std::vector<std::tuple<uint64_t, uint32_t, int64_t>> parseFrames(const std::vector<uint8_t>& inputBuffer, uint64_t fileSize, com::rdk::hal::videodecoder::Codec codec) {
        std::vector<std::tuple<uint64_t, uint32_t, int64_t>> frames; // {offset, size, pts}

        if (mTestConfiguration.frameDetails.empty()) {
            UT_LOG_ERROR("frameDetails is empty");
            return frames;
        }

        switch (codec) {
            case com::rdk::hal::videodecoder::Codec::H264:
            case com::rdk::hal::videodecoder::Codec::H265: {
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
            case com::rdk::hal::videodecoder::Codec::MPEG: {
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

private:
    thread mDecoderThread, mConsumerThread;
    IVideoDecoder::Id mDecoderId;
    sp<IVideoDecoder> mDecoderHandle;
    sp<VideoDecoderControllerListener> mControlListener;
    sp<VideoDecoderEventListener> mEventListener;
    sp<IAVBuffer> mAVBufferManager;
    bool mSecureDecode;
    bool mTunnelMode;
    testConfiguration_t mTestConfiguration;
    deque<InputVideoFrame_t> mInputFrame;
    mutex mDecodeMutex;
    atomic<bool> mDecodingStream;
    bool mConcurrentDecodeConsume;
    mutex mAVbufferHelperLock;

#if 0
    void decoderThreadFunc()
    {
        sp<IVideoDecoderController> videoDecoderController;
        int64_t videoBufferHandle = 0;
        Pool videoPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();
        mDecodingStream = true;

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc Start", getpid(), syscall(SYS_gettid));

        // Validate gAVbufferHelper
        if (!gAVbufferHelper) {
            UT_LOG_ERROR("gAVbufferHelper is null");
            mDecodingStream = false;
            return;
        }
        UT_LOG_INFO("gAVbufferHelper initialized: %p, secureDecode: %d", gAVbufferHelper, mSecureDecode);

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
            return;
        }
        std::ifstream inputFile(mTestConfiguration.input, std::ios::binary);
        if (!inputFile.is_open()) {
            UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
            mDecodingStream = false;
            return;
        }

        inputFile.seekg(0, std::ios::end);
        auto fileSizePos = inputFile.tellg();
        if (fileSizePos < 0) {
            UT_LOG_ERROR("Failed to get file size for %s: tellg returned %lld",
                        mTestConfiguration.input.c_str(), static_cast<int64_t>(fileSizePos));
            mDecodingStream = false;
            return;
        }
        uint64_t fileSize = static_cast<uint64_t>(fileSizePos);
        if (fileSize == 0) {
            UT_LOG_ERROR("Input file is empty: %s", mTestConfiguration.input.c_str());
            mDecodingStream = false;
            return;
        }
        if (fileSize > std::numeric_limits<uint32_t>::max()) {
            UT_LOG_ERROR("File size too large: %llu bytes, max allowed: %u bytes",
                        fileSize, std::numeric_limits<uint32_t>::max());
            mDecodingStream = false;
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
            return;
        }

        // Determine frame data based on codec
        std::vector<std::tuple<uint64_t, uint32_t, int64_t, bool>> frameData; // [offset, size, pts, isErrorFrame]
        if (mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H264 ||
            mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H265) {
            if (mTestConfiguration.frameDetails.empty()) {
                UT_LOG_ERROR("frameDetails is empty for H.264/H.265");
                mDecodingStream = false;
                return;
            }
            uint64_t offset = 0;
            for (const auto& [size, pts, isErrorFrame] : mTestConfiguration.frameDetails) {
                if (offset + size > fileSize) {
                    UT_LOG_ERROR("Frame size at offset %llu exceeds file size %llu", offset + size, fileSize);
                    mDecodingStream = false;
                    return;
                }
                frameData.emplace_back(offset, size, pts, isErrorFrame);
                UT_LOG_INFO("Codec:%d Frame from frameDetails: offset=%llu, size=%u, pts=%lld, errorFrame=%d",
                            mTestConfiguration.codec,offset, size, pts, isErrorFrame);
                offset += size;
            }
        } else {
            // Parse frames for VP9, MPEG, AV1
            auto parsedFrames = parseFrames(inputBuffer, fileSize, mTestConfiguration.codec);
            if (parsedFrames.empty()) {
                UT_LOG_ERROR("No frames parsed for codec: %d", static_cast<int>(mTestConfiguration.codec));
                mDecodingStream = false;
                return;
            }
            for (size_t i = 0; i < parsedFrames.size(); ++i) {
                auto [offset, size, pts] = parsedFrames[i];
                bool isErrorFrame = (i < mTestConfiguration.frameDetails.size()) ?
                                    std::get<2>(mTestConfiguration.frameDetails[i]) : false;
                frameData.emplace_back(offset, size, pts, isErrorFrame);
                UT_LOG_INFO("Parsed frame: Codec:%d offset=%llu, size=%u, pts=%lld, errorFrame=%d",
                            mTestConfiguration.codec,offset, size, pts, isErrorFrame);
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
                std::vector<std::pair<Property, int32_t>> startedIntProps = {
                    {Property::OPERATIONAL_MODE, static_cast<int32_t>(OperationalMode::TUNNELLED)}
                };
                UT_LOG_INFO("Set properties in TUNNELLED Mode");
                VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, startedIntProps, intValue);
            }
            std::optional<PropertyValue> aidl_return;
            VIDEODECODER_GETPROPERTY(mDecoderHandle, Property::OPERATIONAL_MODE, aidl_return);
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
        UT_LOG_INFO("Initial Decoder Metrics: Codec: %d ,Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec,framesDecoded, decodeErrors, framesDropped);

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
                UT_LOG_INFO("Calling writeSecureHandle(handle:0x%llx, size=%u, gAVbufferHelper:%p)",
                            videoBufferHandle, frameSize, gAVbufferHelper);
                if (!gAVbufferHelper || !gAVbufferHelper->writeSecureHandle(videoBufferHandle, inputBuffer.data() + frameOffset, frameSize)) {
                    UT_LOG_ERROR("Failed to copy frame data to secure buffer");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    continue;
                }
            } else {
                std::lock_guard<std::mutex> lock(mAVbufferHelperLock);
                UT_LOG_INFO("Calling mapHandle(handle:0x%llx, gAVbufferHelper:%p)", videoBufferHandle, gAVbufferHelper);
                if (!gAVbufferHelper) {
                    UT_LOG_ERROR("gAVbufferHelper is null before mapHandle");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    continue;
                }
                mapBuffer = gAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
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

            UT_LOG_INFO("mTestConfiguration.stopPts = %lld framePTS = %lld ",mTestConfiguration.stopPts,framePTS);
            UT_LOG_INFO("mTestConfiguration.discontinuityPts = %lld framePTS = %lld ",mTestConfiguration.discontinuityPts,framePTS);
            UT_LOG_INFO("mTestConfiguration.flushPts = %lld framePTS = %lld ",mTestConfiguration.flushPts,framePTS);
            UT_LOG_INFO("mTestConfiguration.flushResetPts = %lld framePTS = %lld ",mTestConfiguration.flushResetPts,framePTS);
            UT_LOG_INFO("mTestConfiguration.eosPts = %lld framePTS = %lld ",mTestConfiguration.eosPts,framePTS);

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
                if (mTestConfiguration.eosPts == framePTS) {
                    UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
                    VIDEODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, videoDecoderController, true);
                    VIDEODECODER_DECODEBUFFER_FAILURE(videoDecoderController, framePTS, videoBufferHandle, Status::EX_NONE);
                    VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
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
                        gAVbufferHelper->unmapHandle(videoBufferHandle);
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
                            mTestConfiguration.codec,framesDecoded, decodeErrors, framesDropped);
            }

            // Check decode time
            auto decodeEnd = std::chrono::steady_clock::now();
            auto decodeTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(decodeEnd - decodeStart).count();
            if (decodeTimeNs > mTestConfiguration.decode_time_threshold) {
                UT_LOG_WARNING("Frame %zu decode time %lld ns exceeds threshold %lld ns",
                            i, decodeTimeNs, mTestConfiguration.decode_time_threshold);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(mTestConfiguration.decoderDelay));

            // Free Buffer
            {
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                UT_LOG_INFO("Freeing buffer handle:0x%llx", videoBufferHandle);
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
            }

            if (!mSecureDecode && mapBuffer) {
                std::lock_guard<std::mutex> lock(mAVbufferHelperLock);
                UT_LOG_INFO("Unmapping buffer handle:0x%llx", videoBufferHandle);
                if (gAVbufferHelper) {
                    gAVbufferHelper->unmapHandle(videoBufferHandle);
                }
            }
        }

        // Get final metrics
        getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec,framesDecoded, decodeErrors, framesDropped);

        // Stop Decoder
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Stopping decoder");
            VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
        }

        getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("After Stop Decoder Metrics: Code: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec,framesDecoded, decodeErrors, framesDropped);

        // Close Decoder
        VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, videoDecoderController, true);

        // Destroy Video Pool
        {
            std::lock_guard<std::mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
        }

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc End", getpid(), syscall(SYS_gettid));
        mDecodingStream = false;
    }
#endif
    void decoderThreadFunc() {
        sp<IVideoDecoderController> videoDecoderController;
        int64_t videoBufferHandle = 0;
        Pool videoPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();
        mDecodingStream = true;

        DecodedVideoFrame_t decodedFrame;

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc Start", getpid(), syscall(SYS_gettid));

        // Validate gAVbufferHelper
        if (!gAVbufferHelper) {
            UT_LOG_ERROR("gAVbufferHelper is null");
            mDecodingStream = false;
            return;
        }
        UT_LOG_INFO("gAVbufferHelper initialized: %p, secureDecode: %d", gAVbufferHelper, mSecureDecode);

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
            return;
        }
        std::ifstream inputFile(mTestConfiguration.input, std::ios::binary);
        if (!inputFile.is_open()) {
            UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
            mDecodingStream = false;
            return;
        }

        inputFile.seekg(0, std::ios::end);
        auto fileSizePos = inputFile.tellg();
        if (fileSizePos < 0) {
            UT_LOG_ERROR("Failed to get file size for %s: tellg returned %lld",
                        mTestConfiguration.input.c_str(), static_cast<int64_t>(fileSizePos));
            mDecodingStream = false;
            return;
        }
        uint64_t fileSize = static_cast<uint64_t>(fileSizePos);
        if (fileSize == 0) {
            UT_LOG_ERROR("Input file is empty: %s", mTestConfiguration.input.c_str());
            mDecodingStream = false;
            return;
        }
        if (fileSize > std::numeric_limits<uint32_t>::max()) {
            UT_LOG_ERROR("File size too large: %llu bytes, max allowed: %u bytes",
                        fileSize, std::numeric_limits<uint32_t>::max());
            mDecodingStream = false;
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
            return;
        }

        // Determine frame data based on codec
        std::vector<std::tuple<uint64_t, uint32_t, int64_t, bool>> frameData;
        if (mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H264 ||
            mTestConfiguration.codec == com::rdk::hal::videodecoder::Codec::H265) {
            if (mTestConfiguration.frameDetails.empty()) {
                UT_LOG_ERROR("frameDetails is empty for H.264/H.265");
                mDecodingStream = false;
                return;
            }
            uint64_t offset = 0;
            for (const auto& [size, pts, isErrorFrame] : mTestConfiguration.frameDetails) {
                if (offset + size > fileSize) {
                    UT_LOG_ERROR("Frame size at offset %llu exceeds file size %llu", offset + size, fileSize);
                    mDecodingStream = false;
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
                std::vector<std::pair<Property, int32_t>> startedIntProps = {
                    {Property::OPERATIONAL_MODE, static_cast<int32_t>(OperationalMode::TUNNELLED)}
                };
                UT_LOG_INFO("Set properties in TUNNELLED Mode");
                VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, startedIntProps, intValue);
            }
            std::optional<PropertyValue> aidl_return;
            VIDEODECODER_GETPROPERTY(mDecoderHandle, Property::OPERATIONAL_MODE, aidl_return);
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
                UT_LOG_INFO("Calling writeSecureHandle(handle:0x%llx, size=%u, gAVbufferHelper:%p)",
                            videoBufferHandle, frameSize, gAVbufferHelper);
                if (!gAVbufferHelper || !gAVbufferHelper->writeSecureHandle(videoBufferHandle, inputBuffer.data() + frameOffset, frameSize)) {
                    UT_LOG_ERROR("Failed to copy frame data to secure buffer");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    continue;
                }
            } else {
                std::lock_guard<std::mutex> lock(mAVbufferHelperLock);
                UT_LOG_INFO("Calling mapHandle(handle:0x%llx, gAVbufferHelper:%p)", videoBufferHandle, gAVbufferHelper);
                if (!gAVbufferHelper) {
                    UT_LOG_ERROR("gAVbufferHelper is null before mapHandle");
                    AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                    continue;
                }
                mapBuffer = gAVbufferHelper->mapHandle(videoBufferHandle, &mapSize);
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
                if (mTestConfiguration.eosPts == framePTS) {
                    UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
                    VIDEODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, videoDecoderController, true);
                    VIDEODECODER_DECODEBUFFER_FAILURE(videoDecoderController, framePTS, videoBufferHandle, Status::EX_NONE);
                    VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
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
                        gAVbufferHelper->unmapHandle(videoBufferHandle);
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

            if (mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(100))) {
                UT_LOG_INFO("Retrieved output frame: PTS=%lld, handle=0x%llx, isMetaAvailable=%d",
                            decodedFrame.framePTS, decodedFrame.frameBufferHandle, decodedFrame.isMetaAvailable);
                if (decodedFrame.isMetaAvailable && decodedFrame.metadata) {
                    UT_LOG_INFO("Validating metadata for PTS=%lld", decodedFrame.framePTS);
                    // Validate resolution
                    UT_ASSERT_EQUAL(decodedFrame.metadata->codedWidth, mTestConfiguration.codedWidth)
                        << "Width mismatch: expected=" << mTestConfiguration.codedWidth
                        << ", actual=" << decodedFrame.metadata->codedWidth;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->codedHeight, mTestConfiguration.codedHeight)
                        << "Height mismatch: expected=" << mTestConfiguration.codedHeight
                        << ", actual=" << decodedFrame.metadata->codedHeight;

                    // Validate active region
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->activeX, mTestConfiguration.activeX)
                        << "activeX mismatch: expected=" << mTestConfiguration.activeX
                        << ", actual=" << decodedFrame.metadata->activeX;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->activeY, mTestConfiguration.activeY)
                        << "activeY mismatch: expected=" << mTestConfiguration.activeY
                        << ", actual=" << decodedFrame.metadata->activeY;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->activeWidth, mTestConfiguration.activeWidth)
                        << "activeWidth mismatch: expected=" << mTestConfiguration.activeWidth
                        << ", actual=" << decodedFrame.metadata->activeWidth;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->activeHeight, mTestConfiguration.activeHeight)
                        << "activeHeight mismatch: expected=" << mTestConfiguration.activeHeight
                        << ", actual=" << decodedFrame.metadata->activeHeight;*/

                    // Validate aspect ratios
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->parX, mTestConfiguration.parX)
                        << "PAR X mismatch: expected=" << mTestConfiguration.parX
                        << ", actual=" << decodedFrame.metadata->parX;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->parY, mTestConfiguration.parY)
                        << "PAR Y mismatch: expected=" << mTestConfiguration.parY
                        << ", actual=" << decodedFrame.metadata->parY;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->sarX, mTestConfiguration.sarX)
                        << "SAR X mismatch: expected=" << mTestConfiguration.sarX
                        << ", actual=" << decodedFrame.metadata->sarX;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->sarY, mTestConfiguration.sarY)
                        << "SAR Y mismatch: expected=" << mTestConfiguration.sarY
                        << ", actual=" << decodedFrame.metadata->sarY;*/

                    // Validate pixel format and depth
                    UT_ASSERT_EQUAL(decodedFrame.metadata->pixelFormat, mTestConfiguration.pixelFormat)
                        << "Pixel format mismatch: expected=" << static_cast<int>(mTestConfiguration.pixelFormat)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->pixelFormat);
                    UT_ASSERT_EQUAL(decodedFrame.metadata->colorDepth, mTestConfiguration.colorDepth)
                        << "Color depth mismatch: expected=" << mTestConfiguration.colorDepth
                        << ", actual=" << decodedFrame.metadata->colorDepth;

                    // Validate dynamic range
                    UT_ASSERT_EQUAL(decodedFrame.metadata->dynamicRange, mTestConfiguration.dynamicRange)
                        << "Dynamic range mismatch: expected=" << static_cast<int>(mTestConfiguration.dynamicRange)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->dynamicRange);

                    // Validate frame rate
                    UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateNumerator, mTestConfiguration.frameRateNumerator)
                        << "Frame rate numerator mismatch: expected=" << mTestConfiguration.frameRateNumerator
                        << ", actual=" << decodedFrame.metadata->frameRateNumerator;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateDenominator, mTestConfiguration.frameRateDenominator)
                        << "Frame rate denominator mismatch: expected=" << mTestConfiguration.frameRateDenominator
                        << ", actual=" << decodedFrame.metadata->frameRateDenominator;

                    // Validate scan type
                    UT_ASSERT_EQUAL(decodedFrame.metadata->scanType, mTestConfiguration.scanType)
                        << "Scan type mismatch: expected=" << static_cast<int>(mTestConfiguration.scanType)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->scanType);

                    // Validate AFD
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->afd, mTestConfiguration.afd)
                        << "AFD mismatch: expected=" << mTestConfiguration.afd
                        << ", actual=" << decodedFrame.metadata->afd;*/

                    // Validate flags
                    UT_ASSERT_EQUAL(decodedFrame.metadata->endOfStream, mTestConfiguration.endOfStream)
                        << "endOfStream mismatch: expected=" << mTestConfiguration.endOfStream
                        << ", actual=" << decodedFrame.metadata->endOfStream;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->discontinuity, mTestConfiguration.discontinuity)
                        << "discontinuity mismatch: expected=" << mTestConfiguration.discontinuity
                        << ", actual=" << decodedFrame.metadata->discontinuity;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->lowLatency, mTestConfiguration.lowLatency)
                        << "lowLatency mismatch: expected=" << mTestConfiguration.lowLatency
                        << ", actual=" << decodedFrame.metadata->lowLatency;

                    // Validate source
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->source, mTestConfiguration.source)
                        << "Source mismatch: expected=" << static_cast<int>(mTestConfiguration.source)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->source);

                    // Optional: SHA1 comparison if you expect it
                    if (!mTestConfiguration.sha1.empty()) {
                        UT_ASSERT_EQUAL(decodedFrame.metadata->sha1.size(), mTestConfiguration.sha1.size())
                            << "SHA1 size mismatch";
                        for (size_t i = 0; i < decodedFrame.metadata->sha1.size(); ++i) {
                            UT_ASSERT_EQUAL(decodedFrame.metadata->sha1[i], mTestConfiguration.sha1[i])
                                << "SHA1 byte mismatch at index " << i;
                        }
                    }*/
                } else {
                    UT_LOG_INFO("No metadata available for PTS=%lld", decodedFrame.framePTS);
                }
                // Release output buffer
                UT_LOG_INFO("Releasing output buffer handle:0x%llx", decodedFrame.frameBufferHandle);
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
            } else {
                UT_LOG_WARNING("No output frame retrieved for input PTS=%lld after 100ms", framePTS);
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
                UT_LOG_INFO("Freeing input buffer handle:0x%llx", videoBufferHandle);
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
            }

            if (!mSecureDecode && mapBuffer) {
                std::lock_guard<std::mutex> lock(mAVbufferHelperLock);
                UT_LOG_INFO("Unmapping buffer handle:0x%llx", videoBufferHandle);
                if (gAVbufferHelper) {
                    gAVbufferHelper->unmapHandle(videoBufferHandle);
                }
            }
        }

        // Retrieve any remaining output frames
        while (mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(100))) {
            UT_LOG_INFO("Retrieved remaining output frame: PTS=%lld, handle=0x%llx, isMetaAvailable=%d",
                        decodedFrame.framePTS, decodedFrame.frameBufferHandle, decodedFrame.isMetaAvailable);
        if (decodedFrame.metadata) {
                    UT_LOG_INFO("Validating metadata for PTS=%lld", decodedFrame.framePTS);
                    // Validate active region
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->activeX, mTestConfiguration.activeX)
                        << "activeX mismatch: expected=" << mTestConfiguration.activeX
                        << ", actual=" << decodedFrame.metadata->activeX;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->activeY, mTestConfiguration.activeY)
                        << "activeY mismatch: expected=" << mTestConfiguration.activeY
                        << ", actual=" << decodedFrame.metadata->activeY;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->activeWidth, mTestConfiguration.activeWidth)
                        << "activeWidth mismatch: expected=" << mTestConfiguration.activeWidth
                        << ", actual=" << decodedFrame.metadata->activeWidth;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->activeHeight, mTestConfiguration.activeHeight)
                        << "activeHeight mismatch: expected=" << mTestConfiguration.activeHeight
                        << ", actual=" << decodedFrame.metadata->activeHeight;*/

                    // Validate aspect ratios
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->parX, mTestConfiguration.parX)
                        << "PAR X mismatch: expected=" << mTestConfiguration.parX
                        << ", actual=" << decodedFrame.metadata->parX;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->parY, mTestConfiguration.parY)
                        << "PAR Y mismatch: expected=" << mTestConfiguration.parY
                        << ", actual=" << decodedFrame.metadata->parY;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->sarX, mTestConfiguration.sarX)
                        << "SAR X mismatch: expected=" << mTestConfiguration.sarX
                        << ", actual=" << decodedFrame.metadata->sarX;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->sarY, mTestConfiguration.sarY)
                        << "SAR Y mismatch: expected=" << mTestConfiguration.sarY
                        << ", actual=" << decodedFrame.metadata->sarY;*/

                    // Validate pixel format and depth
                    UT_ASSERT_EQUAL(decodedFrame.metadata->pixelFormat, mTestConfiguration.pixelFormat)
                        << "Pixel format mismatch: expected=" << static_cast<int>(mTestConfiguration.pixelFormat)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->pixelFormat);
                    UT_ASSERT_EQUAL(decodedFrame.metadata->colorDepth, mTestConfiguration.colorDepth)
                        << "Color depth mismatch: expected=" << mTestConfiguration.colorDepth
                        << ", actual=" << decodedFrame.metadata->colorDepth;

                    // Validate dynamic range
                    UT_ASSERT_EQUAL(decodedFrame.metadata->dynamicRange, mTestConfiguration.dynamicRange)
                        << "Dynamic range mismatch: expected=" << static_cast<int>(mTestConfiguration.dynamicRange)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->dynamicRange);

                    // Validate frame rate
                    UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateNumerator, mTestConfiguration.frameRateNumerator)
                        << "Frame rate numerator mismatch: expected=" << mTestConfiguration.frameRateNumerator
                        << ", actual=" << decodedFrame.metadata->frameRateNumerator;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateDenominator, mTestConfiguration.frameRateDenominator)
                        << "Frame rate denominator mismatch: expected=" << mTestConfiguration.frameRateDenominator
                        << ", actual=" << decodedFrame.metadata->frameRateDenominator;

                    // Validate scan type
                    UT_ASSERT_EQUAL(decodedFrame.metadata->scanType, mTestConfiguration.scanType)
                        << "Scan type mismatch: expected=" << static_cast<int>(mTestConfiguration.scanType)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->scanType);

                    // Validate AFD
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->afd, mTestConfiguration.afd)
                        << "AFD mismatch: expected=" << mTestConfiguration.afd
                        << ", actual=" << decodedFrame.metadata->afd;*/

                    // Validate flags
                    UT_ASSERT_EQUAL(decodedFrame.metadata->endOfStream, mTestConfiguration.endOfStream)
                        << "endOfStream mismatch: expected=" << mTestConfiguration.endOfStream
                        << ", actual=" << decodedFrame.metadata->endOfStream;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->discontinuity, mTestConfiguration.discontinuity)
                        << "discontinuity mismatch: expected=" << mTestConfiguration.discontinuity
                        << ", actual=" << decodedFrame.metadata->discontinuity;
                    UT_ASSERT_EQUAL(decodedFrame.metadata->lowLatency, mTestConfiguration.lowLatency)
                        << "lowLatency mismatch: expected=" << mTestConfiguration.lowLatency
                        << ", actual=" << decodedFrame.metadata->lowLatency;

                    // Validate source
                    /*UT_ASSERT_EQUAL(decodedFrame.metadata->source, mTestConfiguration.source)
                        << "Source mismatch: expected=" << static_cast<int>(mTestConfiguration.source)
                        << ", actual=" << static_cast<int>(decodedFrame.metadata->source);

                    // Optional: SHA1 comparison if you expect it
                    if (!mTestConfiguration.sha1.empty()) {
                        UT_ASSERT_EQUAL(decodedFrame.metadata->sha1.size(), mTestConfiguration.sha1.size())
                            << "SHA1 size mismatch";
                        for (size_t i = 0; i < decodedFrame.metadata->sha1.size(); ++i) {
                            UT_ASSERT_EQUAL(decodedFrame.metadata->sha1[i], mTestConfiguration.sha1[i])
                                << "SHA1 byte mismatch at index " << i;
                        }
                    }*/
                } else {
                    UT_LOG_INFO("No metadata available for PTS=%lld", decodedFrame.framePTS);
                }
            UT_LOG_INFO("Releasing output buffer handle:0x%llx", decodedFrame.frameBufferHandle);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
        }

        // Get final metrics
        getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    mTestConfiguration.codec, framesDecoded, decodeErrors, framesDropped);

        UT_ASSERT_EQUAL(framesDecoded, mTestConfiguration.no_of_frames)
                        << "Frames Decoded mismatch: expected=" << mTestConfiguration.no_of_frames
                        << ", actual=" << framesDecoded;
        UT_ASSERT_EQUAL(decodeErrors, mTestConfiguration.decodeErrors)
                        << "Frames Decoded mismatch: expected=" << mTestConfiguration.decodeErrors
                        << ", actual=" << decodeErrors;
        UT_ASSERT_EQUAL(framesDropped, mTestConfiguration.framesDropped)
                        << "Frames Decoded mismatch: expected=" << mTestConfiguration.framesDropped
                        << ", actual=" << framesDropped;

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
            UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
            AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
        }

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc End", getpid(), syscall(SYS_gettid));
        mDecodingStream = false;
    }
    void consumerThreadFunc()
    {
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
                std::lock_guard<std::mutex> lock(mDecodeMutex);
                for (auto it = mInputFrame.begin(); it != mInputFrame.end(); ++it) {
                    if (it->framePTS == decodedFrame.framePTS) {
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
                /*UT_ASSERT_EQUAL(decodedFrame.metadata->pixelFormat, mTestConfiguration.pixelFormat)
                    << "Pixel format mismatch: expected=" << static_cast<int>(mTestConfiguration.pixelFormat)
                    << ", actual=" << static_cast<int>(decodedFrame.metadata->pixelFormat);
                UT_ASSERT_EQUAL(decodedFrame.metadata->dynamicRange, mTestConfiguration.dynamicRange)
                    << "Dynamic range mismatch: expected=" << static_cast<int>(mTestConfiguration.dynamicRange)
                    << ", actual=" << static_cast<int>(decodedFrame.metadata->dynamicRange);*/
                UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateNumerator, mTestConfiguration.frameRateNumerator)
                    << "Frame rate numerator mismatch: expected=" << mTestConfiguration.frameRateNumerator
                    << ", actual=" << decodedFrame.metadata->frameRateNumerator;
                UT_ASSERT_EQUAL(decodedFrame.metadata->frameRateDenominator, mTestConfiguration.frameRateDenominator)
                    << "Frame rate denominator mismatch: expected=" << mTestConfiguration.frameRateDenominator
                    << ", actual=" << decodedFrame.metadata->frameRateDenominator;
                /*UT_ASSERT_EQUAL(decodedFrame.metadata->colorDepth, mTestConfiguration.colorDepth)
                    << "Color depth mismatch: expected=" << mTestConfiguration.colorDepth
                    << ", actual=" << decodedFrame.metadata->colorDepth;*/
            } else {
                UT_LOG_WARNING("No metadata available for PTS=%lld", decodedFrame.framePTS);
            }

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                decodedFrame.receiveTime - inputFrame.sendTime).count();
            UT_LOG_INFO("Decode Time in msec [PTS: %lld]: %lld", decodedFrame.framePTS, duration / 1000);
            /*UT_ASSERT_TRUE(duration <= mTestConfiguration.decode_time_threshold / 1000)
                << "Decode time " << duration << " us exceeds threshold "
                << mTestConfiguration.decode_time_threshold / 1000 << " us";*/
        }

        if (errorCount > 0) {
            UT_LOG_INFO("No. of Error frames pushed: %d", errorCount);
            UT_LOG_INFO("No. of Error frames Callbacks: %d", mEventListener->getErrors().size());
            UT_ASSERT_EQUAL(mEventListener->getErrors().size(), errorCount);
        }

        UT_LOG_INFO("[PID: %d, TID: %d] consumerThreadFunc end", getpid(), syscall(SYS_gettid));
    }

    void decodeAndConsumeVideoStreams()
    {
        sp<IVideoDecoderController> videoDecoderController;
        int64_t videoBufferHandle;
        Pool videoPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

        UT_LOG_INFO("decodeAndConsumeVideoStreams start:");

        /* Create Video Pool */
        AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, videoPool);

        ifstream inputFile(mTestConfiguration.input, ios::binary);
        if (!inputFile)
        {
            UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
            return;
        }


        /* Open the Decoder */
        VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, videoDecoderController, true);

        /* Start the decoder */
        VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);

        for (const auto& [frameSize, framePTS, errorFrame] : mTestConfiguration.frameDetails)
        {
            vector<uint8_t> inputBuffer;
            size_t output_buffer_size;
            double rms_diff = 0.0;
            int32_t trimEndNs = 0;
            int32_t trimStartNs = 0;
            int32_t startFramesDecoded, startFrameErrors, startFramesDropped;
            int32_t endFramesDecoded, endFrameErrors, endFramesDropped;
            DecodedVideoFrame_t decodedFrame;

            chrono::steady_clock::time_point startTime;

            /* Allocate the video buffer */
            AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, videoPool, frameSize, videoBufferHandle);

            inputBuffer.resize(frameSize);
            inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

            startTime = chrono::steady_clock::now();
            VIDEODECODER_DECODEBUFFER_SUCCESS(videoDecoderController, framePTS, videoBufferHandle);

            mControlListener->getNextFrame(decodedFrame, std::chrono::milliseconds(100));

            /* Verify the decoded buffer in non-tunnel mode */
            if (!mTunnelMode && !decodedFrame.data.empty())
            {
                //output_buffer_size = decodedFrame.metadata.codedWidth * decodedFrame.metadata.codedHeight * vts_bytesPerPixel(decodedFrame.metadata.pixelFormat);

                //rms_diff = vts_computeVideoRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), decodedFrame.metadata.width , decodedFrame.metadata.height, decodedFrame.metadata.pixelFormat);
                UT_ASSERT_TRUE(rms_diff <= mTestConfiguration.rms_diff_threashold);
            }

            auto duration = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - startTime);
            UT_LOG_INFO("Decode Time [PTS: %lld]: %lld", framePTS, duration.count());

            startFramesDecoded++;

            if (errorFrame)
            {
                startFrameErrors++;
            }

            /* Validate the decoder metrics */
            UT_ASSERT_TRUE(startFramesDecoded == endFramesDecoded);
            UT_ASSERT_TRUE(startFrameErrors == endFrameErrors);
            UT_ASSERT_TRUE(startFramesDropped == endFramesDropped);

            /* Free the video buffer */
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
        }

        /* Stop the decoder */
        VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);

        /* Close the decoder */
        VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, videoDecoderController, true);

        /* Destroy video pool */
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
    }
};

class VTS_L3_VideoDecoder : public UTCore
{
public:
    VTS_L3_VideoDecoder() : UTCore() {}

    ~VTS_L3_VideoDecoder() override = default;

    vector<tuple<IVideoDecoder::Id, sp<IVideoDecoder>, sp<VideoDecoderControllerListener>, sp<VideoDecoderEventListener>, videodecoder::Capabilities *>> mVideoDecoderList;
    sp<IAVBuffer> mAVBufferManager;

    void SetUp() override
    {
        vector<IVideoDecoder::Id> decoderIds;

        mAVBufferManager = getAVBufferService();
        gTestConfiguration = vts_kvp_create_instance(VTS_TEST_CONFIG_PATH);

        gAVbufferHelper = getAVBufferHelperInstance();

        /* Get the Video decoder IDs */
        VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

        android::ProcessState::self()->startThreadPool();

        for (const auto& _decoderId : decoderIds)
        {
            sp<IVideoDecoder> videoDecoder;
            videodecoder::Capabilities *capabilities = new videodecoder::Capabilities();
            sp<VideoDecoderEventListener> eventListener = new VideoDecoderEventListener();
            sp<VideoDecoderControllerListener> controlListener = new VideoDecoderControllerListener();

            /* Get Video decoder handle */
            VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

            /* Register events */
            VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

            /* Get Capabilities of Video Decoder */
            VIDEODECODER_GETCAPABILITIES(videoDecoder, capabilities);

            mVideoDecoderList.push_back({_decoderId, videoDecoder, controlListener, eventListener, capabilities});
        }
    }

    void runSequentialDecode(testConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            HeapMetrics beforeHeapMetrics, afterHeapMetrics;
            int32_t startFramesDecoded, startFrameErrors, startFramesDropped;
            int32_t endFramesDecoded, endFrameErrors, endFramesDropped;

            if (secureDecode && !capabilities->supportsSecure)
            {
                UT_LOG_INFO("Decoder [%d] doesn't support secure decoding", _decoderId);
                continue;
            }

            auto result = std::find_if(capabilities->supportedCodecs.begin(), capabilities->supportedCodecs.end(),
                [&testConfig](const com::rdk::hal::videodecoder::CodecCapabilities& cap) {
                    return cap.codec == testConfig.codec;
                });
            if (result == capabilities->supportedCodecs.end())
            {
                UT_LOG_INFO("Decoder [%d] doesn't support Codec [%s]", _decoderId, videodecoder::toString(testConfig.codec).c_str());
                continue;
            }


            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

            DecoderTest decodeTest(_decoderId, decoderHandle, controlListener,
                                   eventListener, mAVBufferManager, secureDecode, tunnelMode, testConfig);

            decodeTest.run(concurrentDecodeConsume);

            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

            /* Check if the decoder freed all the buffer pools */
            UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);
        }
    }

#if 0
    void runConcurrentDecode(testConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        std::vector<std::thread> threads;
        std::vector<DecoderTest> decoderTestList;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;

        /* Get heap metrics before decoding */
        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            // Skip if the platform doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                UT_LOG_INFO("Decoder [%d] doesn't support secure decoding", _decoderId.value);
                continue;
            }

            // Check if the codec is supported
            auto result = std::find_if(capabilities->supportedCodecs.begin(), capabilities->supportedCodecs.end(),
                [&testConfig](const CodecCapabilities& cap) { return cap.codec == testConfig.codec; });
            if (result == capabilities->supportedCodecs.end())
            {
                UT_LOG_INFO("Decoder [%d] doesn't support Codec [%s]", _decoderId.value, toString(testConfig.codec).c_str());
                continue;
            }

            // Emplace DecoderTest directly into vector to avoid copying
            decoderTestList.emplace_back(_decoderId, decoderHandle, controlListener,
                                        eventListener, mAVBufferManager, secureDecode, tunnelMode, testConfig);
            threads.emplace_back(&DecoderTest::run, &decoderTestList.back(), concurrentDecodeConsume);
        }

        // Wait for all threads to complete
        for (auto& t : threads)
        {
            t.join();
        }

        // Check decoder metrics for each test
        for (auto& decoderTest : decoderTestList)
        {
            int32_t framesDecoded, decodeErrors, framesDropped;
            bool metricsSuccess = decoderTest.getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
            UT_ASSERT_TRUE(metricsSuccess) << "Failed to get decoder metrics for decoder ID ";
            UT_LOG_INFO("Decoder Metrics  Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                        framesDecoded, decodeErrors, framesDropped);

            // Validate metrics
            UT_ASSERT_TRUE(framesDecoded >= 0) << "Invalid frames decoded count";
            UT_ASSERT_TRUE(decodeErrors >= 0) << "Invalid decode errors count";
            UT_ASSERT_TRUE(framesDropped >= 0) << "Invalid frames dropped count";

            // Validate error frames against testConfig
            if (!testConfig.frameDetails.empty())
            {
                int32_t expectedErrorFrames = std::count_if(testConfig.frameDetails.begin(), testConfig.frameDetails.end(),
                    [](const auto& frame) { return std::get<2>(frame); });
                UT_ASSERT_TRUE(decodeErrors >= expectedErrorFrames) << "Expected at least " << expectedErrorFrames << " decode errors";
            }
        }

        /* Get heap metrics after decoding */
        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

        /* Check if the decoder freed all buffer pools */
        UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure) << "Secure heap mismatch";
        UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed) << "Heap bytes used mismatch";
        UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal) << "Heap bytes total mismatch";
    }

#endif

#if 1
    void runConcurrentDecode(testConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        std::vector<std::unique_ptr<DecoderTest>> decoderTests;
        std::vector<std::future<void>> futures;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;

        // Get heap metrics before decoding
        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

        size_t activeDecoders = 0;
        decoderTests.reserve(mVideoDecoderList.size());
        futures.reserve(mVideoDecoderList.size());

        UT_LOG_INFO("Launching decoding for codec: %s, secure: %d, tunnel: %d, concurrentDecodeConsume: %d",
                    videodecoder::toString(testConfig.codec).c_str(),
                    secureDecode, tunnelMode, concurrentDecodeConsume);

        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList) {
            auto it = std::find_if(gVideodecoders_hfp.begin(), gVideodecoders_hfp.end(),
                [_decoderId](const decoder& item) { return item.id == _decoderId.value; });
            if (it == gVideodecoders_hfp.end()) {
                UT_LOG_INFO("[DecoderID: %d] Not available", _decoderId.value);
                continue;
            }

            if (secureDecode && !capabilities->supportsSecure) {
                UT_LOG_INFO("[DecoderID: %d] Doesn't support secure decoding", _decoderId.value);
                continue;
            }

            auto result = std::find_if(capabilities->supportedCodecs.begin(), capabilities->supportedCodecs.end(),
                                    [&testConfig](const CodecCapabilities& cap) { return cap.codec == testConfig.codec; });
            if (result == capabilities->supportedCodecs.end()) {
                UT_LOG_INFO("[DecoderID: %d] Doesn't support Codec [%s]", _decoderId.value,
                            videodecoder::toString(testConfig.codec).c_str());
                continue;
            }

            auto decoderTest = std::make_unique<DecoderTest>(_decoderId, decoderHandle, controlListener, eventListener,
                                                            mAVBufferManager, secureDecode, tunnelMode, testConfig);
            futures.push_back(std::async(std::launch::async, &DecoderTest::run, decoderTest.get(), concurrentDecodeConsume));
            decoderTests.push_back(std::move(decoderTest));
            activeDecoders++;
        }

        if (activeDecoders == 0) {
            UT_LOG("ERROR", "No decoders support the requested configuration (codec: %s, secure: %d)",
                videodecoder::toString(testConfig.codec).c_str(), secureDecode);
            UT_ASSERT_TRUE(false) << "No valid decoders found";
        }

        // Wait for all decoding tasks to complete
        for (auto& f : futures) {
            try {
                f.get();
            } catch (const std::exception& e) {
                UT_LOG("ERROR", "Exception in decoder thread: %s", e.what());
                UT_ASSERT_TRUE(false) << "Decoder thread failed";
            }
        }

        // Validate decoder metrics
        for (const auto& decoderTest : decoderTests) {
            int32_t framesDecoded, decodeErrors, framesDropped;
            bool metricsSuccess = decoderTest->getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
            UT_ASSERT_TRUE(metricsSuccess) << "Failed to get decoder metrics for decoder ID ";
            UT_LOG_INFO("Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                         framesDecoded, decodeErrors, framesDropped);

            // Validate metrics
            size_t expectedFrames = testConfig.frameDetails.size();
            UT_ASSERT_TRUE(framesDecoded == static_cast<int32_t>(expectedFrames))
                << "Frame count mismatch: expected=" << expectedFrames << ", actual=" << framesDecoded;

            int32_t expectedErrorFrames = std::count_if(testConfig.frameDetails.begin(),
                                                        testConfig.frameDetails.end(),
                                                        [](const auto& frame) { return std::get<2>(frame); });
            UT_ASSERT_TRUE(decodeErrors == expectedErrorFrames)
                << "Decode errors mismatch: expected=" << expectedErrorFrames << ", actual=" << decodeErrors;

            UT_ASSERT_TRUE(framesDropped == 0) << "Unexpected frames dropped: " << framesDropped;
        }

        // Get heap metrics after decoding
        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

        // Check if the decoder freed all buffer pools
        if (beforeHeapMetrics.secure != afterHeapMetrics.secure ||
            beforeHeapMetrics.bytesUsed != afterHeapMetrics.bytesUsed ||
            beforeHeapMetrics.bytesTotal != afterHeapMetrics.bytesTotal) {
            UT_LOG("ERROR", "Heap metrics mismatch: "
                "secure(before=%d, after=%d), bytesUsed(before=%zu, after=%zu), bytesTotal(before=%zu, after=%zu)",
                beforeHeapMetrics.secure, afterHeapMetrics.secure,
                beforeHeapMetrics.bytesUsed, afterHeapMetrics.bytesUsed,
                beforeHeapMetrics.bytesTotal, afterHeapMetrics.bytesTotal);
            UT_ASSERT_TRUE(false) << "Heap metrics mismatch";
        }
    }
#endif
    void runTest(char *testName, bool secureMode, bool tunnelMode, bool concurrentMode)
    {
        vector<testConfiguration_t> testConfiguration;

        UT_LOG_INFO("start parsing concurrentMode %d ",concurrentMode);
        vts_videodecoder_parse_testconfig(gTestConfiguration, VIDEODECODERMGR, gTestGroup, testName, testConfiguration);
        UT_LOG_INFO("End parsing");

        if (concurrentMode)
        {
            for (auto& test : testConfiguration)
            {
                runConcurrentDecode(test, secureMode, tunnelMode, true);
            }
        }
        else
        {
            for (auto& test : testConfiguration)
            {
                runSequentialDecode(test, secureMode, tunnelMode, true);
            }
        }
    }

    void TearDown() override
    {
        for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            /* Unregister the events */
            VIDEODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
        }
        mVideoDecoderList.clear();
    }
};

/* Test Case Definitions based on provided feature list (30 test cases) */

/**
 * @brief Test the ability to decode video frames in sequential decoding
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 001
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 002
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_Error)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with Graphics option
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 003
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_Graphics)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_Graphics";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 004
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_Discontinuity)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 005
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_EOS)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 006
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_Flush)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 007
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_FlushReset)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 008
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure)
{

    char *testName = "Decode_Concurrent_NonTunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 009
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Tunnel_NonSecure)
{

    char *testName = "Decode_Tunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 010
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_Secure)
{

    char *testName = "Decode_NonTunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test tunnel and secure modes together
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 011
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Tunnel_Secure)
{

    char *testName = "Decode_Tunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test tunnel mode with error streams
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 012
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Tunnel_NonSecure_Error)
{

    char *testName = "Decode_Tunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test tunnel mode with trim
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 013
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Tunnel_NonSecure_Trim)
{

    char *testName = "Decode_Tunnel_NonSecure_Trim";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test secure mode with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 014
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_Secure_Error)
{

    char *testName = "Decode_NonTunnel_Secure_Error";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test secure mode with trim
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 015
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_Secure_Trim)
{

    char *testName = "Decode_NonTunnel_Secure_Trim";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test error streams with discontinuity
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 016
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Error_Discontinuity)
{

    char *testName = "Decode_NonTunnel_NonSecure_Error_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test error streams with EOS
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 017
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Error_EOS)
{

    char *testName = "Decode_NonTunnel_NonSecure_Error_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trim with discontinuity
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 018
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Trim_Discontinuity)
{

    char *testName = "Decode_NonTunnel_NonSecure_Trim_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test discontinuity with EOS
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 019
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Discontinuity_EOS)
{

    char *testName = "Decode_NonTunnel_NonSecure_Discontinuity_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 020
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Flush_Error)
{

    char *testName = "Decode_NonTunnel_NonSecure_Flush_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset and error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 021
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_FlushReset_Error)
{

    char *testName = "Decode_NonTunnel_NonSecure_FlushReset_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in tunnel mode
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 022
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure)
{

    char *testName = "Decode_Concurrent_Tunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in secure mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 023
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure)
{

    char *testName = "Decode_Concurrent_NonTunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with error streams
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 024
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Error)
{

    char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with trim
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 025
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Trim)
{

    char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Trim";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with discontinuity
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 026
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Discontinuity)
{

    char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with EOS
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 027
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_EOS)
{

    char *testName = "Decode_Concurrent_NonTunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 028
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Flush)
{

    char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush and reset
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 029
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_FlushReset)
{

    char *testName = "Decode_Concurrent_NonTunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}


/** @} */ // End of VIDEO_DECODER_TEST_L3 File
/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK