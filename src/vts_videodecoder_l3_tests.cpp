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

UT_ADD_TEST_TO_GROUP(VTS_L3_VideoDecoder, UT_TESTS_L3)

static char *gTestGroup = "L3";

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
        vector<IVideoDecoder::Id> decoderIds;

        mAVBufferManager = getAVBufferService();
        gTestConfiguration = vts_kvp_create_instance(VTS_TEST_CONFIG_PATH);

        mAVbufferHelper = getAVBufferHelperInstance();

        VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

        android::ProcessState::self()->startThreadPool();

        for (const auto& _decoderId : decoderIds)
        {
            sp<IVideoDecoder> videoDecoder;
            videodecoder::Capabilities *capabilities = new videodecoder::Capabilities();
            sp<VideoDecoderEventListener> eventListener = new VideoDecoderEventListener();
            sp<VideoDecoderControllerListener> controlListener = new VideoDecoderControllerListener();

            VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder);

            VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, eventListener);

            VIDEODECODER_GETCAPABILITIES(videoDecoder, capabilities);

            mVideoDecoderList.push_back({_decoderId, videoDecoder, controlListener, eventListener, capabilities});
        }
    }

    void runSequentialDecode(testConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mVideoDecoderList)
        {
            HeapMetrics beforeHeapMetrics, afterHeapMetrics;

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

            DecoderTest decodeTest(_decoderId, decoderHandle, controlListener,
                                   eventListener, mAVBufferManager, secureDecode, tunnelMode, testConfig,mAVbufferHelper);

            decodeTest.run(concurrentDecodeConsume);

            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

            UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);
        }
    }

    void runConcurrentDecode(testConfiguration_t& testConfig, bool secureDecode, bool tunnelMode, bool concurrentDecodeConsume)
    {
        std::vector<std::unique_ptr<DecoderTest>> decoderTests;
        std::vector<std::future<void>> futures;
        HeapMetrics beforeHeapMetrics, afterHeapMetrics;

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
                                                            mAVBufferManager, secureDecode, tunnelMode, testConfig,mAVbufferHelper);
            futures.push_back(std::async(std::launch::async, &DecoderTest::run, decoderTest.get(), concurrentDecodeConsume));
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

    void runTest(char *testName, bool secureMode, bool tunnelMode, bool concurrentMode)
    {
        vector<testConfiguration_t> testConfiguration;

        UT_LOG_INFO("start parsing concurrentMode %d ", concurrentMode);
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
 * @brief Test decoding with GOP option
 *        Decoding type: Sequential, Modes: Non-Tunnel, Non-Secure
 * **Test Group ID:** 01 **Test Case ID:** 003
 */
UT_ADD_TEST(VTS_L3_VideoDecoder, Decode_Sequential_NonTunnel_NonSecure_GOP)
{

    char *testName = "Decode_Sequential_NonTunnel_NonSecure_GOP";
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

/** @} */ // End of VIDEO_DECODER_HALTEST_L3 File
/** @} */ // End of VIDEO_DECODER_HALTEST
/** @} */ // End of VIDEO_DECODER Module
/** @} */ // End of HPK