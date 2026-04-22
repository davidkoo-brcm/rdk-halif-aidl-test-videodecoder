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
 * @file vts_videodecoder_l3_test.cpp
 *
 * @brief Contains L3 test cases for the Video Decoder HAL.
 */

#include "vts_videodecoder.h"

#include <vector>
#include <future>
#include <memory>

using namespace std;

int32_t VideoDecoderEventListener::mUsedDecoderID = -1;

UT_ADD_TEST_TO_GROUP(VTS_L3_VideoDecoder, UT_TESTS_L3)

static char *gTestGroup = "L3";

static sp<IAVBuffer> vts_getAVBufferService()
{
    return
        android::interface_cast<IAVBuffer>(
            android::defaultServiceManager()->getService(
                android::String16(IAVBuffer::serviceName().c_str())
            )
        );
}

class VTS_L3_VideoDecoder : public UTCore
{
public:
    VTS_L3_VideoDecoder() : UTCore() {}

    ~VTS_L3_VideoDecoder() override = default;

    vector<tuple<IVideoDecoder::Id, sp<IVideoDecoder>, sp<VideoDecoderControllerListener>, sp<VideoDecoderEventListener>, videodecoder::Capabilities *>> mVideoDecoderList;
    sp<IAVBuffer> mAVBufferManager;
    IAVBufferHelper* mAVbufferHelper;

    void SetUp() override
    {
        mAVBufferManager = vts_getAVBufferService();

        if (mAVBufferManager == nullptr) {
            UT_FAIL_FATAL("AVBuffer Service not available");
        }

        if (gVideoDecoderManager == nullptr) {
            UT_FAIL_FATAL("VideoDecoder Service not available");
        }
        vector<IVideoDecoder::Id> decoderIds;

        // Parse HFP to populate gVideodecoders_hfp
        UT_status_t status = vts_videodecoder_parseVideoDecoderHFP();
        if (status != UT_STATUS_OK) {
            UT_LOG_ERROR("Failed to parse video decoder HFP");
            UT_ASSERT_TRUE(false) << "HFP parsing failed";
            return;
        }

        if (gVideodecoders_hfp.empty()) {
            UT_LOG_ERROR("No valid decoders found in HFP");
            UT_ASSERT_TRUE(false) << "No decoders in HFP";
            return;
        }

        mAVbufferHelper = getAVBufferHelperInstance();
        if (mAVbufferHelper == nullptr) {
            UT_FAIL_FATAL("AV Buffer Helper not available");
        }

        VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

        android::ProcessState::self()->startThreadPool();

        // Filter decoder IDs based on gVideodecoders_hfp
        for (const auto& _decoderId : decoderIds)
        {
            int decoderIdValue = _decoderId.value; // Assuming IVideoDecoder::Id has a 'value' field
            auto it = std::find_if(gVideodecoders_hfp.begin(), gVideodecoders_hfp.end(),
                [decoderIdValue](const decoder& item) { return item.id == decoderIdValue; });
            if (it == gVideodecoders_hfp.end()) {
                UT_LOG_WARNING("Decoder ID %d found in system but not in HFP, skipping", decoderIdValue);
                continue;
            }
            sp<IVideoDecoder> videoDecoder;
            videodecoder::Capabilities *capabilities = new videodecoder::Capabilities();
            sp<VideoDecoderEventListener> eventListener = new VideoDecoderEventListener();
            sp<VideoDecoderControllerListener> controlListener = new VideoDecoderControllerListener();

            VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

            VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

            VIDEODECODER_GETCAPABILITIES(videoDecoder, capabilities);

            mVideoDecoderList.push_back({_decoderId, videoDecoder, controlListener, eventListener, capabilities});
            UT_LOG_INFO("Added decoder ID %d to test list", decoderIdValue);
        }
        if (mVideoDecoderList.empty()) {
            UT_LOG_ERROR("No valid decoders found matching HFP configuration");
            UT_ASSERT_TRUE(false) << "No valid decoders available";
        }
    }

    void runSequentialDecode(videoDecoderTestConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            HeapMetrics beforeHeapMetrics, afterHeapMetrics;
            bool isSinkAttached = false;

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

            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

            videoDecoderTest decodeTest(_decoderId, decoderHandle, controlListener,
                                   eventListener, mAVBufferManager, secureDecode, tunnelMode, testConfig,mAVbufferHelper, isSinkAttached);

            decodeTest.run(concurrentDecodeConsume);

            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

            UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);
        }
    }

    void runConcurrentDecode(videoDecoderTestConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        std::vector<std::unique_ptr<videoDecoderTest>> decoderTests;
        std::vector<std::future<void>> futures;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;
        bool isSinkAttached = false;

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

            auto decoderTest = std::make_unique<videoDecoderTest>(_decoderId, decoderHandle, controlListener, eventListener,
                                                            mAVBufferManager, secureDecode, tunnelMode, testConfig,mAVbufferHelper, isSinkAttached);
            futures.push_back(std::async(std::launch::async, &videoDecoderTest::run, decoderTest.get(), concurrentDecodeConsume));
            decoderTests.push_back(std::move(decoderTest));
            activeDecoders++;
        }

        if (activeDecoders == 0) {
            UT_LOG("ERROR", "No decoders support the requested configuration (codec: %s, secure: %d)",
                videodecoder::toString(testConfig.codec).c_str(), secureDecode);
            UT_ASSERT_TRUE(false) << "No valid decoders found";
        }

        for (auto& f : futures) {
            try {
                f.get();
            } catch (const std::exception& e) {
                UT_LOG("ERROR", "Exception in decoder thread: %s", e.what());
                UT_ASSERT_TRUE(false) << "Decoder thread failed";
            }
        }

        for (const auto& decoderTest : decoderTests) {
            int32_t framesDecoded, decodeErrors, framesDropped;
            bool metricsSuccess = decoderTest->getDecoderMetrics(framesDecoded, decodeErrors, framesDropped);
            UT_ASSERT_TRUE(metricsSuccess) << "Failed to get decoder metrics for decoder ID ";
            UT_LOG_INFO("Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
                        framesDecoded, decodeErrors, framesDropped);

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

        AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

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

    void runTest(const char *testName, bool secureMode, bool tunnelMode, bool concurrentMode)
    {
        vector<videoDecoderTestConfiguration_t> testConfiguration;

        UT_LOG_INFO("start parsing concurrentMode %d ", concurrentMode);
        vts_videodecoder_parse_testconfig(gVideoDecoderTestConfiguration, VIDEODECODERMGR, gTestGroup, testName, testConfiguration);
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
            VIDEODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
        }
        mVideoDecoderList.clear();
    }
};

/**
 * @brief Test the ability to decode video frames in sequential decoding
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 001
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure";
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
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP option
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 003
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_GOP)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_GOP";
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
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Discontinuity";
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
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_EOS";
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
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Flush";
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
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FlushReset";
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
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

#if 0
/**
 * @brief Test decoding in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 009
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_Secure)
{
    const char *testName = "Decode_NonTunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test secure mode with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 010
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_NonTunnel_Secure_Error";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test secure mode with GOP
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 011
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_Secure_GOP)
{
    const char *testName = "Decode_NonTunnel_Secure_GOP";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}
#endif
/**
 * @brief Test error streams with discontinuity
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 012
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Error_Discontinuity)
{
    const char *testName = "Decode_NonTunnel_NonSecure_Error_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test error streams with EOS
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 013
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Error_EOS)
{
    const char *testName = "Decode_NonTunnel_NonSecure_Error_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test discontinuity with EOS
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 014
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Discontinuity_EOS)
{
    const char *testName = "Decode_NonTunnel_NonSecure_Discontinuity_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 015
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_Flush_Error)
{
    const char *testName = "Decode_NonTunnel_NonSecure_Flush_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset and error streams
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 016
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_NonTunnel_NonSecure_FlushReset_Error)
{
    const char *testName = "Decode_NonTunnel_NonSecure_FlushReset_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

#if 0
/**
 * @brief Test concurrent decoding in secure mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 017
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}
#endif
/**
 * @brief Test concurrent decoding with error streams
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 018
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Error)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with discontinuity
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 019
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with EOS
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 020
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_EOS)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 021
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding with flush and reset
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 022
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 042
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_GOPStructure";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

#if 0
/**
 * @brief Test decoding with codec support
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 023
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different resolutions
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 024
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_Resolutions)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Resolutions";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 025
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 026
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 027
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with adaptive bitrate streaming
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 028
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_AdaptiveBitrate)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_AdaptiveBitrate";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}


/**
 * @brief Test decoding with codec support in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 029
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_CODEC)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_CODEC";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 030
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 031
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_EOS";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 032
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Flush";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 033
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_FlushReset";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different resolutions in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 034
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_Resolutions)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_Resolutions";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 035
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_HDRModes)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_HDRModes";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 036
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_FrameRate)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_FrameRate";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in secure mode
 *        Decoding type: Sequential, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 037
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_Secure_LowLatency)
{
    const char *testName = "Decode_Sequential_NonTunnel_Secure_LowLatency";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 038
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different resolutions in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 039
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_Resolutions)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_Resolutions";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 040
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 041
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}




/**
 * @brief Test decoding in low latency mode in concurrent mode
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 043
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Concurrent_NonTunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 044
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Error";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 045
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_CODEC)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_CODEC";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 046
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 047
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_EOS";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 048
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Flush";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 049
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_FlushReset";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different resolutions in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 050
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_Resolutions)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_Resolutions";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 051
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_HDRModes)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_HDRModes";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 052
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_FrameRate)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_FrameRate";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 053
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_GOPStructure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 054
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_LowLatency)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_LowLatency";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent and secure modes
 *        Decoding type: Concurrent, Modes: Non-Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 054
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_NonTunnel_Secure_LowLatency)
{
    const char *testName = "Decode_Concurrent_NonTunnel_Secure_LowLatency";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 055
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Tunnel_NonSecure)
{
    const char *testName = "Decode_Tunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test tunnel and secure modes together
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 056
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Tunnel_Secure)
{
    const char *testName = "Decode_Tunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test tunnel mode with error streams
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 057
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Tunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in tunnel mode
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 058
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 059
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 060
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 061
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 062
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 063
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different resolutions in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 064
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_Resolutions)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_Resolutions";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 065
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 066
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 067
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_GOPStructure)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_GOPStructure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in tunnel mode
 *        Decoding type: Sequential, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 068
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Sequential_Tunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 069
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_Error)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_Error";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 070
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_CODEC)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_CODEC";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 071
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 072
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_EOS)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_EOS";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 073
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_Flush)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_Flush";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 074
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_FlushReset)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_FlushReset";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different resolutions in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 075
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_Resolutions)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_Resolutions";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 076
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_HDRModes)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_HDRModes";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 077
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_FrameRate)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_FrameRate";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 078
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_GOPStructure)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_GOPStructure";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in tunnel and secure modes
 *        Decoding type: Sequential, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 079
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_Tunnel_Secure_LowLatency)
{
    const char *testName = "Decode_Sequential_Tunnel_Secure_LowLatency";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with error streams in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 080
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_Error";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with codec support in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 081
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_CODEC)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_CODEC";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 082
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS signal in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 083
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_EOS";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush functionality in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 084
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_Flush";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush with reset functionality in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 085
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_FlushReset";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different resolutions in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 086
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_Resolutions)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_Resolutions";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with HDR modes in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 087
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_HDRModes)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_HDRModes";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with different frame rates in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 088
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_FrameRate)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_FrameRate";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding with GOP structure in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 089
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_GOPStructure)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_GOPStructure";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoding in low latency mode in concurrent and tunnel modes
 *        Decoding type: Concurrent, Modes: Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 090
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_NonSecure_LowLatency)
{
    const char *testName = "Decode_Concurrent_Tunnel_NonSecure_LowLatency";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decoding in tunnel and secure modes
 *        Decoding type: Concurrent, Modes: Tunnel, Secure
 * **Test Group ID:** 01 **Test Case ID:** 091
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Concurrent_Tunnel_Secure)
{
    const char *testName = "Decode_Concurrent_Tunnel_Secure";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}
#endif

/**
 * @brief Test that setColorimetry correctly propagates BT2020_ST2084 colorimetry
 *        to decoded frame metadata when an HDR10 HEVC stream is decoded.
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 092
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_Clrmty)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_Clrmty";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test that when setColorimetry is not called (colorimetry=UNKNOWN),
 *        decoded frame metadata reports UNKNOWN colorimetry.
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 093
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_ClrmtyNoSet)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_ClrmtyNoSet";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test that setMasteringDisplayInfo correctly propagates HDR10 mastering
 *        display metadata to decoded frame metadata when an HDR10 HEVC stream is decoded.
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 094
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_MstrDisplay)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MstrDisplay";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test that when setMasteringDisplayInfo is not called,
 *        decoded frame metadata reports nullopt for masteringDisplayInfo.
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 095
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_MstrDisplayNoSet)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_MstrDisplayNoSet";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test that setContentLightLevel correctly propagates MaxCLL/MaxFALL
 *        to decoded frame metadata when an HDR10 HEVC stream is decoded.
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 096
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_CntntLight)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_CntntLight";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test that when setContentLightLevel is not called,
 *        decoded frame metadata reports nullopt for contentLightLevel.
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 097
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_CntntLightNoSet)
{
    const char *testName = "Decode_Sequential_NonTunnel_NonSecure_CntntLightNoSet";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runTest(testName, secureMode, tunnelMode, concurrentMode);
}

/** @} */ // End of VIDEO_DECODER_HALTEST_L3 File
/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK
