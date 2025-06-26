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

using namespace std::chrono;
using namespace std;


/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L3_VideoDecoder, UT_TESTS_L3)


static char *gTestGroup = "L3";


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
                bool secureDecode, bool tunnelMode, testConfiguration_t& config)
    {
        mDecoderId = decoderId;
        mDecoderHandle = decoderHandle;
        mControlListener = controlListener;
        mSecureDecode = secureDecode;
        mTunnelMode = tunnelMode;
        mEventListener = eventListener;
        mTestConfiguration = config;
        mAVBufferManager = AVBufferManager;
        mDecodingStream = false;
        mAVbufferHelper = getAVBufferHelperInstance();
    }

    void run(bool concurrentDecodeConsume)
    {
        mConcurrentDecodeConsume = concurrentDecodeConsume;
        mDecodingStream = true;
        if (concurrentDecodeConsume)
        {
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
    IAVBufferHelper* mAVbufferHelper;
    mutex mAVbufferHelperLock;
#if 0
    void decoderThreadFunc()
    {
        sp<IVideoDecoderController> videoDecoderController;
        int64_t videoBufferHandle;
        Pool videoPool;
        sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();

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

        int32_t framesDecoded, decodeErrors, framesDropped;
        this->getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("Decoder Metrics Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                             framesDecoded, decodeErrors, framesDropped);


        for (const auto& [frameSize, framePTS, errorFrame] : mTestConfiguration.frameDetails)
        {
            vector<uint8_t> inputBuffer;
            InputVideoFrame_t inputFrame;

            /* Allocate the video buffer */
            AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, videoPool, frameSize, videoBufferHandle);

            inputBuffer.resize(frameSize);
            inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);

            inputFrame.sendTime = chrono::steady_clock::now();
            inputFrame.framePTS = framePTS;
            inputFrame.reference = mTestConfiguration.reference;
            inputFrame.rms_threshold = mTestConfiguration.rms_diff_threashold;
            inputFrame.isErrorFrame = errorFrame;
            {
                lock_guard<mutex> lock(mDecodeMutex);
                mInputFrame.push_back(inputFrame);
            }

            /* ReStart the decoder */
            if (mTestConfiguration.stopPts == framePTS)
            {
                VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
                VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
            }

            if (mTestConfiguration.discontinuityPts == framePTS)
            {
                VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, videoDecoderController, true);
            }

            if (mTestConfiguration.flushPts == framePTS)
            {
                VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle,videoDecoderController, false,true);
            }

            if (mTestConfiguration.flushResetPts == framePTS)
            {
                VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle,videoDecoderController, true,true);
                VIDEODECODER_START_SUCCESS(mDecoderHandle,videoDecoderController, true);
            }

            if (mTestConfiguration.eosPts == framePTS)
            {
                /* After EOS signal decode should fail */
                VIDEODECODER_DECODEBUFFER_FAILURE(videoDecoderController, framePTS, videoBufferHandle, Status::EX_NONE);

                /* Decoder should be restarted after EOS */
                VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
                VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
            }

            VIDEODECODER_DECODEBUFFER_SUCCESS(videoDecoderController, framePTS, videoBufferHandle);
            this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));
            /* Free the video buffer */
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
        }

        /* Stop the decoder */
        VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);

        /* Close the decoder */
        VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, videoDecoderController, true);

        /* Destroy video pool */
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);

        UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc End", getpid(), syscall(SYS_gettid));

        mDecodingStream = false;
    }
#endif

void decoderThreadFunc()
{
    sp<IVideoDecoderController> videoDecoderController;
    int64_t videoBufferHandle = 0;
    Pool videoPool;
    sp<IAVBufferSpaceListener> bufferListener = new AVBufferSpaceListener();
    mDecodingStream = true;

    UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc Start", getpid(), syscall(SYS_gettid));

    if (!mAVbufferHelper)
    {
        UT_LOG_ERROR("mAVbufferHelper is null");
        mDecodingStream = false;
        return;
    }
    UT_LOG_INFO("mAVbufferHelper initialized: %p, secureDecode: %d", mAVbufferHelper, mSecureDecode);

    /* Create Video Pool */
    {
        lock_guard<mutex> lock(mDecodeMutex);
        UT_LOG_INFO("Calling createVideoPool(secureHeap:%d, videoDecoderId:%d, listener:%p)",
                    mSecureDecode, mDecoderId, bufferListener.get());
        AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, videoPool);
        UT_LOG_INFO("Created video pool with handle:%lld", videoPool.handle);
    }

    ifstream inputFile(mTestConfiguration.input, ios::binary);
    if (!inputFile)
    {
        UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
        mDecodingStream = false;
        return;
    }

    inputFile.seekg(0, ios::end);
    uint64_t fileSize = static_cast<uint64_t>(inputFile.tellg());
    inputFile.seekg(0, ios::beg);
    if (fileSize == 0)
    {
        UT_LOG_ERROR("Input file is empty: %s", mTestConfiguration.input.c_str());
        mDecodingStream = false;
        return;
    }
    UT_LOG_INFO("Input file size: %llu bytes", fileSize);

    vector<uint8_t> inputBuffer(fileSize);
    inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), fileSize);
    if (!inputFile)
    {
        UT_LOG_ERROR("Failed to read file data");
        mDecodingStream = false;
        return;
    }
    uint64_t actualSize = static_cast<uint64_t>(inputFile.gcount());
    if (actualSize != fileSize)
    {
        UT_LOG_WARNING("Read %llu bytes, expected %llu bytes", actualSize, fileSize);
        fileSize = actualSize;
    }

    /* Open Decoder */
    {
        lock_guard<mutex> lock(mDecodeMutex);
        UT_LOG_INFO("Calling open(codec:%d, secure:%d, listener:%p)",
                    mTestConfiguration.codec, mSecureDecode, mControlListener.get());
        VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, videoDecoderController, true);
        UT_LOG_INFO("Decoder opened, controller:%p", videoDecoderController.get());
    }

    /* Start Decoder */
    {
        lock_guard<mutex> lock(mDecodeMutex);
        UT_LOG_INFO("Calling start()");
        VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
        UT_LOG_INFO("Decoder started");
    }

    int32_t framesDecoded, decodeErrors, framesDropped;
    this->getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
    UT_LOG_INFO("Decoder Metrics Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                framesDecoded, decodeErrors, framesDropped);

    /* Parse H.264 */
    vector<pair<uint64_t, uint32_t>> frames; // {offset, size}
    uint64_t pos = 0;
    while (pos < fileSize - 4)
    {
        bool isStartCode = false;
        uint32_t startCodeLen = 0;
        if (pos < fileSize - 3 && inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 && inputBuffer[pos + 2] == 1)
        {
            isStartCode = true;
            startCodeLen = 3;
        }
        else if (pos < fileSize - 4 && inputBuffer[pos] == 0 && inputBuffer[pos + 1] == 0 &&
                 inputBuffer[pos + 2] == 0 && inputBuffer[pos + 3] == 1)
        {
            isStartCode = true;
            startCodeLen = 4;
        }

        if (isStartCode)
        {
            uint64_t start = pos;
            pos += startCodeLen;
            uint64_t nextPos = pos;
            while (nextPos < fileSize - 3)
            {
                if ((inputBuffer[nextPos] == 0 && inputBuffer[nextPos + 1] == 0 && inputBuffer[nextPos + 2] == 1) ||
                    (nextPos < fileSize - 4 && inputBuffer[nextPos] == 0 && inputBuffer[nextPos + 1] == 0 &&
                     inputBuffer[nextPos + 2] == 0 && inputBuffer[nextPos + 3] == 1))
                {
                    break;
                }
                nextPos++;
            }
            uint32_t frameSize = nextPos - start;
            if (frameSize > startCodeLen)
            {
                frames.emplace_back(start, frameSize);
                UT_LOG_INFO("Frame detected: offset=%llu, size=%u", start, frameSize);
            }
            pos = nextPos;
        }
        else
        {
            pos++;
        }
    }
    if (frames.empty() || frames.size() != 6)
    {
        UT_LOG_ERROR("Invalid H.264 frames: found %zu, expected ", frames.size());
        mDecodingStream = false;
        //return;
    }
    UT_LOG_INFO("Found %zu H.264 frames", frames.size());

    /* Process Frames */
    const int64_t PTS_INCREMENT_NS = 40000000; // 25 fps
    for (size_t i = 0; i < frames.size(); ++i)
    {
        auto [frameOffset, frameSize] = frames[i];
        int64_t framePTS = i * PTS_INCREMENT_NS;

        UT_LOG_INFO("Processing frame %zu: offset=%llu, size=%u, PTS=%lldns", i, frameOffset, frameSize, framePTS);

        /* Allocate Buffer */
        {
            lock_guard<mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Calling alloc(poolHandle:%lld, size:%u)", videoPool.handle, frameSize);
            videoBufferHandle = 0;
            AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, videoPool, frameSize, videoBufferHandle);
            UT_LOG_INFO("Allocated buffer handle:0x%llx", videoBufferHandle);
        }

        /*if (videoBufferHandle <= 0 || videoBufferHandle == IAVBuffer::INVALID_HANDLE || videoBufferHandle > 0xFFFFFFFF)
        {
            UT_LOG_ERROR("Invalid buffer handle: 0x%llx", videoBufferHandle);
            continue;
        }*/

        /* Write Frame Data */
        void* mapBuffer = nullptr;
        uint32_t mapSize = 0;
        if (mSecureDecode)
        {
            lock_guard<mutex> lock(mAVbufferHelperLock);
            UT_LOG_INFO("Calling writeSecureHandle(handle:0x%llx, size:%u, mAVbufferHelper:%p)",
                        videoBufferHandle, frameSize, mAVbufferHelper);
            if (!mAVbufferHelper || !mAVbufferHelper->writeSecureHandle(videoBufferHandle, inputBuffer.data() + frameOffset, frameSize))
            {
                UT_LOG_ERROR("Failed to copy frame data to secure buffer");
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                continue;
            }
        }
        else
        {
            lock_guard<mutex> lock(mAVbufferHelperLock);
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
                UT_LOG_INFO("Copying frame data to mapped buffer, size:%u", frameSize);
                memcpy(mapBuffer, inputBuffer.data() + frameOffset, frameSize);
            }
            else
            {
                UT_LOG_ERROR("Failed to map buffer or insufficient size (mapBuffer:%p, mapSize:%u, required:%u)",
                             mapBuffer, mapSize, frameSize);
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                continue;
            }
        }

        /* Decoder Operations */
        {
            lock_guard<mutex> lock(mDecodeMutex);
            if (mTestConfiguration.stopPts == framePTS)
            {
                UT_LOG_INFO("Stopping decoder at PTS:%lld", framePTS);
                VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
                VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
            }
            if (mTestConfiguration.discontinuityPts == framePTS)
            {
                UT_LOG_INFO("Signaling discontinuity at PTS:%lld", framePTS);
                VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, videoDecoderController, true);
            }
            if (mTestConfiguration.flushPts == framePTS)
            {
                UT_LOG_INFO("Flushing decoder at PTS:%lld", framePTS);
                VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, videoDecoderController, false, true);
            }
            if (mTestConfiguration.flushResetPts == framePTS)
            {
                UT_LOG_INFO("Flush reset at PTS:%lld", framePTS);
                VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, videoDecoderController, true, true);
                VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
            }
            if (mTestConfiguration.eosPts == framePTS)
            {
                UT_LOG_INFO("Signaling EOS at PTS:%lld", framePTS);
                VIDEODECODER_DECODEBUFFER_FAILURE(videoDecoderController, framePTS, videoBufferHandle, Status::EX_NONE);
                VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
                VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);
            }

            InputVideoFrame_t inputVideo;
            inputVideo.sendTime = chrono::steady_clock::now();
            inputVideo.framePTS = framePTS;
            inputVideo.reference = mTestConfiguration.reference;
//            inputVideo.rms_threshold = mTestConfiguration.rms_diff_threshold;
            inputVideo.isErrorFrame = false;
            UT_LOG_INFO("Adding frame to mInputFrame, PTS:%lld", framePTS);
            mInputFrame.push_back(inputVideo);
        }

        /* Decode Frame */
        {
            lock_guard<mutex> lock(mDecodeMutex);
            if (!videoDecoderController)
            {
                UT_LOG_ERROR("videoDecoderController is null");
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
                if (!mSecureDecode && mapBuffer)
                {
                    UT_LOG_INFO("Unmapping buffer handle:0x%llx", videoBufferHandle);
                    mAVbufferHelper->unmapHandle(videoBufferHandle);
                }
                continue;
            }
            UT_LOG_INFO("Calling decodeBuffer(PTS:%lld, bufferHandle:0x%llx)", framePTS, videoBufferHandle);
            VIDEODECODER_DECODEBUFFER_SUCCESS(videoDecoderController, framePTS, videoBufferHandle);
        }
        this->getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
        UT_LOG_INFO("after  Decoder Metrics Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                    framesDecoded, decodeErrors, framesDropped);
        this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));

        /* Free Buffer */
        {
            lock_guard<mutex> lock(mDecodeMutex);
            UT_LOG_INFO("Freeing buffer handle:0x%llx", videoBufferHandle);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, videoBufferHandle);
        }

        if (!mSecureDecode && mapBuffer)
        {
            lock_guard<mutex> lock(mAVbufferHelperLock);
            UT_LOG_INFO("Unmapping buffer handle:0x%llx", videoBufferHandle);
            if (mAVbufferHelper)
            {
                mAVbufferHelper->unmapHandle(videoBufferHandle);
            }
        }
    }

    this->getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
    UT_LOG_INFO("before stop Decoder Metrics Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                framesDecoded, decodeErrors, framesDropped);
    /* Stop Decoder */
    {
        lock_guard<mutex> lock(mDecodeMutex);
        UT_LOG_INFO("Stopping decoder");
        VIDEODECODER_STOP_SUCCESS(mDecoderHandle, videoDecoderController, true);
    }

    /* Close Decoder */
    /* Close the decoder */
        VIDEODECODER_CLOSE_SUCCESS(mDecoderHandle, videoDecoderController, true);
    /* Destroy Video Pool */
    {
        lock_guard<mutex> lock(mDecodeMutex);
        UT_LOG_INFO("Destroying video pool handle:%lld", videoPool.handle);
        AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, videoPool);
    }

    UT_LOG_INFO("[PID: %d, TID: %d] decoderThreadFunc End", getpid(), syscall(SYS_gettid));
    mDecodingStream = false;
}

    void consumerThreadFunc()
    {
        ifstream reference_file;
         int32_t errorCount = 0;

        UT_LOG_INFO("[PID: %d, TID: %d] consumerThreadFunc Start", getpid(), syscall(SYS_gettid));
        while (mDecodingStream || !mInputFrame.empty())
        {
            DecodedVideoFrame_t decodedFrame;
            InputVideoFrame_t inputFrame;

            if (mInputFrame.empty())
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }

            inputFrame = move(mInputFrame.front());
            mInputFrame.pop_front();

            if(inputFrame.isErrorFrame)
            {
                errorCount++;
                continue;
            }

            this_thread::sleep_for(chrono::milliseconds(10));
            continue;

            //mControlListener->getNextFrame(decodedFrame);

            if (!reference_file.is_open())
            {
                reference_file.open(inputFrame.reference);
            }

            if (decodedFrame.framePTS != inputFrame.framePTS)
            {
                UT_LOG_ERROR("Input frame and decoded frame PTS do not match");
                UT_ASSERT_TRUE(false);
                continue;
            }

            /* Verify the decoded buffer in non-tunnel mode */
            if (!mTunnelMode && !decodedFrame.data.empty())
            {
                vector<uint8_t> referenceBuffer;
                double rms_diff;
                int32_t output_buffer_size = decodedFrame.metadata.width * decodedFrame.metadata.height * vts_bytesPerPixel(decodedFrame.metadata.pixelFormat);
                referenceBuffer.resize(output_buffer_size);
                reference_file.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                rms_diff = vts_computeVideoRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), decodedFrame.metadata.width , decodedFrame.metadata.height, decodedFrame.metadata.pixelFormat);
                UT_ASSERT_TRUE(rms_diff <= inputFrame.rms_threshold);
            }

            auto duration = chrono::duration_cast<chrono::microseconds>(decodedFrame.receiveTime - inputFrame.sendTime);
            UT_LOG_INFO("Decode Time in msec [PTS: %lld]: %lld", decodedFrame.framePTS, duration.count());
        }
        if (errorCount > 0)
        {
            //TODO: check for correct validation
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

        /* Create Video Pool */
        AVBUFFER_CREATE_VIDEOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, bufferListener, videoPool);

        ifstream inputFile(mTestConfiguration.input, ios::binary);
        if (!inputFile)
        {
            UT_LOG_ERROR("Failed to open file: %s", mTestConfiguration.input.c_str());
            return;
        }

        ifstream referenceFile(mTestConfiguration.reference, ios::binary);
        if (!referenceFile)
        {
            UT_LOG_ERROR("Failed to open reference file: %s", mTestConfiguration.reference.c_str());
            return;
        }

        /* Open the Decoder */
        VIDEODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, videoDecoderController, true);

        /* Start the decoder */
        VIDEODECODER_START_SUCCESS(mDecoderHandle, videoDecoderController, true);

        for (const auto& [frameSize, framePTS, errorFrame] : mTestConfiguration.frameDetails)
        {
            vector<uint8_t> inputBuffer;
            vector<uint8_t> referenceBuffer;
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

            //mControlListener->getNextFrame(decodedFrame);

            /* Verify the decoded buffer in non-tunnel mode */
            if (!mTunnelMode && !decodedFrame.data.empty())
            {
                output_buffer_size = decodedFrame.metadata.width * decodedFrame.metadata.height * vts_bytesPerPixel(decodedFrame.metadata.pixelFormat);
                referenceBuffer.resize(output_buffer_size);
                referenceFile.read(reinterpret_cast<char*>(referenceBuffer.data()), output_buffer_size);

                rms_diff = vts_computeVideoRMSDiff(referenceBuffer.data(), decodedFrame.data.data(), decodedFrame.metadata.width , decodedFrame.metadata.height, decodedFrame.metadata.pixelFormat);
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
#if 1
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
#endif
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
        for (const auto& decoderTest : decoderTestList)
        {
            int32_t framesDecoded, decodeErrors, framesDropped;
            bool metricsSuccess = decoderTest.getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
            UT_ASSERT_TRUE(metricsSuccess) << "Failed to get decoder metrics for decoder ID " << decoderTest.getDecoderId().value;
            UT_LOG_INFO("Decoder Metrics [ID: %d]: Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                        decoderTest.getDecoderId().value, framesDecoded, decodeErrors, framesDropped);

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

    void runTest(char *testName, bool secureMode, bool tunnelMode, bool concurrentMode)
    {
        vector<testConfiguration_t> testConfiguration;

        UT_LOG_INFO("start parsing");
        vts_videodecoder_parse_testconfig(gTestConfiguration, VIDEODECODERMGR, gTestGroup, testName, testConfiguration);
        UT_LOG_INFO("End parsing");

        if (concurrentMode)
        {
            for (auto& test : testConfiguration)
            {
                runSequentialDecode(test, secureMode, tunnelMode, true);
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
 * @brief Test decoding with trim option
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 003
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_Trim)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_Trim";
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