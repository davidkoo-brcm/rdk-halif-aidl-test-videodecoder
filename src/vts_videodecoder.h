/**
 *  If not stated otherwise in this file or this component's LICENSE
 *  file the following copyright and licenses apply:
 *
 *  Copyright 2025 RDK Management
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an AS IS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 * @par The Hardware Porting Kit
 * HPK is the next evolution of the well-defined Hardware Abstraction Layer
 * (HAL), but augmented with more comprehensive documentation and test suites
 * that OEM or SOC vendors can use to self-certify their ports before taking
 * them to RDKM for validation or to an operator for final integration and
 * deployment. The Hardware Porting Kit effectively enables an OEM and/or SOC
 * vendor to self-certify their own Video Accelerator devices, with minimal RDKM
 * assistance.
 *
 */

/**
 * @addtogroup Video_Decoder Video Decoder Module
 * @{
 */

/**
 * @addtogroup Video_Decoder_Halif_Test Video Decoder HALIF Tests
 * @{
 */

/**
 * @defgroup Video_Decoder_Halif_Test_Header Video Decoder HALIF Tests Header File
 * @{
 * @parblock
 *
 *
 * **Pre-Conditions:**  None@n
 * **Dependencies:** None@n
 *
 * Ref to API Definition specification documentation :
 *
 * @endparblock
 */


/**
 * @file vts_videodecoder.h
 *
 */

#ifndef __VTS_VIDEODECODER_H__
#define __VTS_VIDEODECODER_H__

#include <ut.h>
#include <ut_kvp_profile.h>
#include <ut_log.h>
#include <ut_control_plane.h>
#include <deque>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <deque>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <memory> // For std::unique_ptr
#include <inttypes.h>

#include "com/rdk/hal/avbuffer/IAVBuffer.h"
#include "com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h"
//#include "com/rdk/hal/avbufferhelper.h"
#include "IAVBufferHelper.h"

#include "vts_avbuffer.h"
#include "vts_helper_functions.h"

// Undefine UT_LOG_INFO from ut_log.h to avoid redefinition conflicts
#undef UT_LOG_INFO

#ifdef ENABLE_UT_INFO_LOGGING
// Enable INFO logging using ut_log.h's definition
#define UT_LOG_INFO(format, ...) UT_logPrefix(__FILE__, __LINE__, UT_LOG_ASCII_CYAN "INFO  " UT_LOG_ASCII_NC, format, ## __VA_ARGS__)
#else
#define UT_LOG_INFO(format, ...) // Disable INFO logging by default (no-op)
#endif


#include "com/rdk/hal/videodecoder/IVideoDecoderManager.h"
#include "com/rdk/hal/videodecoder/IVideoDecoderController.h"
#include "com/rdk/hal/videodecoder/IVideoDecoderEventListener.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderControllerListener.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderEventListener.h"
#include "com/rdk/hal/videodecoder/DynamicRange.h"
#include "com/rdk/hal/videodecoder/Codec.h"
#include "com/rdk/hal/videodecoder/CodecCapabilities.h"
#include "com/rdk/hal/videodecoder/OperationalMode.h"

#define VIDEODECODERMGR "IVideoDecoderManager"
#define VIDEODECODERMGR_IVIDEODECODER "IVideoDecoderManager.IVideoDecoder"

using namespace com::rdk::hal;
using namespace com::rdk::hal::videodecoder;
using namespace com::rdk::hal::avbuffer;


using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

static constexpr int VIDEO_MAX_TRANSITORY_STATE_WAIT_MS = 10;
static constexpr int VIDEO_MAX_FINAL_STATE_WAIT_MS = 500;

// Global test data (consider moving to a singleton or test fixture if shared state is needed)

extern sp<IVideoDecoderManager>                  gVideoDecoderManager;
extern std::vector<IVideoDecoder::Id>            decoderIds;
extern std::vector<sp<IVideoDecoder>>            decoders;
extern std::vector<sp<IVideoDecoderController>>  controllers;

#define VIDEO_DECODER_TEST_CONFIG_PATH "./videodecoder_test_configuration/vts_l3_VideoDecoder_test_config.yaml"
#define VIDEO_DECODER_STREAM_CONFIG_PATH "./videodecoder_test_configuration/vts_stream_list.yaml"
#define VIDEO_DECODER_STREAM_DOWNLOAD_PATH "IVideoDecoderManager.stream_download_path"
#define VIDEO_DECODER_STREAM_INPUT_PATH "IVideoDecoderManager.input_streams_path"
#define VIDEO_DECODER_STREAM_REF_PATH "IVideoDecoderManager.reference_streams_path"

extern void *gVideoDecoderTestConfiguration;


typedef struct InputVideoFrame
{
    int64_t framePTS;
    std::string reference;
    double rms_threshold;
    bool isErrorFrame;
    std::chrono::steady_clock::time_point sendTime;
}InputVideoFrame_t;

struct CodecCapability {
    videodecoder::Codec codec;
    int32_t maxFrameRate;
    int32_t maxFrameWidth;
    int32_t maxFrameHeight;
};

typedef struct decoder {
    int32_t id;
    std::vector<CodecCapability> supportedCodecs;
    std::vector<DynamicRange> supportedDynamicRanges;
    std::vector<OperationalMode> supportedOperationalModes;
    bool supportsSecure;
}decoder_t;

extern std::vector<decoder_t> gVideodecoders_hfp;

struct videoFrameMetadata {
        PixelFormat pixelFormat;
        int32_t codedWidth;
        int32_t codedHeight;
        int colorDepth;
        videodecoder::Codec sourceCodec;
        int frameRateNumerator;
	    int frameRateDenominator;
        DynamicRange dynamicRange;
        int bit_rate;
        bool lowLatency;
        bool endOfStream;
        bool discontinuity;
        ScanType scanType;
        AVSource source;
};

struct DecodedVideoFrame_t {
    std::vector<uint8_t> data;
    int64_t framePTS;
    std::chrono::steady_clock::time_point receiveTime;
    bool isMetaAvailable;
    std::unique_ptr<com::rdk::hal::videodecoder::FrameMetadata> metadata;
    bool validationStatus; // Added for validation result
    int64_t frameBufferHandle; // Added for output buffer handle
};

struct videoDecoderTestConfiguration_t {
    std::string input; // Path to input .es file
    std::string streamName; //
    videodecoder::Codec codec; // H264_AVC, HEVC, etc.
    int32_t codedWidth; // Video width
    int32_t codedHeight; // Video height
    uint32_t frameRateNumerator; // Frame rate numerator
    uint32_t frameRateDenominator; // Frame rate denominator
    videodecoder::PixelFormat pixelFormat; // YUV, etc.
    videodecoder::DynamicRange dynamicRange; // SDR, HDR, etc.
    videodecoder::ScanType scanType;
    uint32_t colorDepth; // Bit depth
    uint32_t bit_rate; // Bit rate
    uint32_t no_of_frames; // Total frames
    uint32_t decodeErrors;
    uint32_t framesDropped;
    std::vector<std::tuple<int32_t, int64_t, bool>> frameDetails; // {size, PTS, isErrorFrame}
    std::vector<int64_t> expectedPts; // Added for PTS validation
    double rms_diff_threshold; // RMS difference threshold
    uint32_t decoderDelay; // Delay between decode calls (ms)
    int64_t stopPts; // Stop/restart PTS
    int64_t discontinuityPts; // Discontinuity PTS
    int64_t flushPts; // Flush PTS
    int64_t flushResetPts; // Flush with reset PTS
    int64_t eosPts; // EOS PTS
    int64_t decode_time_threshold; // Decode time budget per frame (ns)
    double rms_diff_threashold; // RMS difference threshold
    bool endOfStream;
    bool discontinuity;
    bool lowLatency;

    // Sink control fields
    int64_t sink_discardFramesUntilPts;
    int64_t sink_underflowPts;
    int64_t sink_resumeTriggerPts;
    int32_t sink_repeatPerFrame;
    int64_t sink_flush_pts;
    int64_t sink_flush_reset_pts;
    int64_t sink_eos_pts;
    int64_t sink_interFrameDelayMs;
    int64_t sink_discontinuityPTS;
    std::string sink_testName;
};


UT_status_t getVideoDecoderService();

/* Status::Exception */
const static ut_control_keyStringMapping_t statusException_mapTable[] =
{
    {(char*)"EX_NONE",                  (int32_t) Status::EX_NONE},
    {(char*)"EX_SECURITY",              (int32_t) Status::EX_SECURITY},
    {(char*)"EX_BAD_PARCELABLE",        (int32_t) Status::EX_BAD_PARCELABLE},
    {(char*)"EX_ILLEGAL_ARGUMENT",      (int32_t) Status::EX_ILLEGAL_ARGUMENT},
    {(char*)"EX_NULL_POINTER",          (int32_t) Status::EX_NULL_POINTER},
    {(char*)"EX_ILLEGAL_STATE",         (int32_t) Status::EX_ILLEGAL_STATE},
    {(char*)"EX_NETWORK_MAIN_THREAD",   (int32_t) Status::EX_NETWORK_MAIN_THREAD},
    {(char*)"EX_UNSUPPORTED_OPERATION", (int32_t) Status::EX_UNSUPPORTED_OPERATION},
    {(char*)"EX_SERVICE_SPECIFIC",      (int32_t) Status::EX_SERVICE_SPECIFIC},
    {(char*)"EX_PARCELABLE",            (int32_t) Status::EX_PARCELABLE},
    {(char*)"EX_HAS_REPLY_HEADER",      (int32_t) Status::EX_HAS_REPLY_HEADER},
    {(char*)"EX_TRANSACTION_FAILED",    (int32_t) Status::EX_TRANSACTION_FAILED},
    {NULL, -1}
};

/* bool */
const static ut_control_keyStringMapping_t bool_mapTable[] =
{
    {(char*)"false", (int32_t) false},
    {(char*)"true",  (int32_t) true},
    {NULL, -1}
};

DynamicRange videoDecoderStringToDynamicRange(const std::string& rangeStr);
OperationalMode videoDecoderStringToOperationalMode(const std::string& modeStr);
PixelFormat videoDecoderStringToPixelFormat(const std::string& formatStr);
bool vts_videodecoder_helper_validatePropertyValue(int decoder_id,const std::optional<PropertyValue>& propertyValue, const videodecoder::Property property);
void vts_videodecoder_helper_validateCapabilities(int32_t decoderId,const videodecoder::Capabilities& capabilities);
double vts_computeVideoRMSDiff(const uint8_t* ref, const uint8_t* dec, size_t width, size_t height, PixelFormat format);

UT_status_t vts_videodecoder_parse_testconfig(void* pTestInstance, const char *module, const char *testGroup, const char *testName, std::vector<videoDecoderTestConfiguration_t>& testConfiguration);

UT_status_t vts_videodecoder_parseVideoDecoderHFP();

size_t vts_bytesPerPixel(PixelFormat format);


#define VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoderIds(OUT:decoderIds:[])"); \
        status = gVideoDecoderManager->getVideoDecoderIds(&decoderIds); \
        UT_LOG_INFO("Result getVideoDecoderIds(OUT:decoderIds:[size: %d]), status:[isOK:%s, exception:%s]", \
                                decoderIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_FALSE_FATAL(decoderIds.empty()); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define VIDEODECODER_MANAGER_GETVIDEODECODER(_decoderId, videoDecoder) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[])", _decoderId.value); \
        status = gVideoDecoderManager->getVideoDecoder(_decoderId, &videoDecoder); \
        UT_LOG_INFO("Result getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, videoDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_TRUE_FATAL(videoDecoder.get() != nullptr); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

/**
 * @brief Calls getVideoDecoder with a valid ID and verifies success.
 * @param[in] gVideoDecoderManager The IVideoDecoderManager instance (assumed available in scope).
 * @param[in] id The IVideoDecoder::Id to query.
 * @param[out] decoder The returned IVideoDecoder instance.
 */
#define VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(_decoderId, videoDecoder) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoder(IN:id:[%d], OUT:videoDecoder[])", (_decoderId).value); \
        status = gVideoDecoderManager->getVideoDecoder(_decoderId, &videoDecoder); \
        UT_LOG_INFO("Result getVideoDecoder(IN:id:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                    (_decoderId).value, (videoDecoder).get(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()) << "Failed to call getVideoDecoder for ID " << (_decoderId).value; \
        UT_ASSERT_TRUE_FATAL((videoDecoder) != nullptr) << "Returned videoDecoder is null for ID " << (_decoderId).value; \
    } while (0)

#define VIDEODECODER_MANAGER_GETVIDEODECODER_FAILURE(_decoderId, videoDecoder) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[])", _decoderId.value); \
        status = gVideoDecoderManager->getVideoDecoder(_decoderId, &videoDecoder); \
        UT_LOG_INFO("Result getVideoDecoder(IN:_decoderId:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, videoDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_TRUE(videoDecoder.get() == nullptr); \
    } while (0)

/**
 * @brief Calls getSupportedOperationalModes and verifies a non-empty list is returned.
 * @param[out] modes Vector to store the supported operational modes.
 * @pre gVideoDecoderManager is initialized.
 * @post modes contains at least one mode (tunnelled or non-tunnelled).
 */
#define VIDEODECODER_MANAGER_GETSUPPORTEDOPERATIONALMODES(modes) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getSupportedOperationalModes(OUT:modes:[])"); \
        status = gVideoDecoderManager->getSupportedOperationalModes(&modes); \
        UT_LOG_INFO("Result getSupportedOperationalModes(OUT:modes:[size: %d]), status:[isOK:%s, exception:%s]", \
                    modes.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_FALSE_FATAL(modes.empty()) << "At least one operational mode must be supported"; \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

/**
 * @brief Calls getCapabilities and verifies the result.
 * @param[in] decoder The IVideoDecoder instance.
 * @param[out] capabilities Pointer to the Capabilities struct to be filled.
 */
#define VIDEODECODER_GETCAPABILITIES(videoDecoder, capabilities) \
     do { \
         Status status; \
         UT_LOG_INFO("Calling getCapabilities(OUT:capabilities)"); \
         status = videoDecoder->getCapabilities(capabilities); \
         UT_LOG_INFO("Result getCapabilities(OUT:capabilities), status:[isOK:%s, exception:%s]", \
                     UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                     UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
         UT_ASSERT_TRUE(status.isOk()); \
         UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
     } while (0)

#define VIDEODECODER_GET_PROPERTY_FAILURE(videoDecoder, property, propertyValue, exception) \
    do { \
        Status status; \
        State state; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", videoDecoder::toString(property).c_str()); \
        status = (videoDecoder)->getProperty(property, &propertyValue); \
        UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", videoDecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

/**
 * @brief Calls getProperty, verifies the result, and checks for intValue tag.
 * @param[in] decoder The IVideoDecoder instance.
 * @param[in] property The property to query.
 * @param[out] aidl_return The optional PropertyValue result.
 */
#define VIDEODECODER_GETPROPERTY(videoDecoder, property, aidl_return) \
do { \
    Status status; \
    UT_LOG_INFO("Calling getProperty(IN:property:[%d], OUT:_aidl_return[]) ", static_cast<int32_t>(property)); \
    status = (videoDecoder)->getProperty((property), &(aidl_return)); \
    UT_LOG_INFO("Result getProperty(IN:property:[%d], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]", \
                static_cast<int32_t>(property), (aidl_return).has_value(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
    UT_ASSERT_TRUE(status.isOk()) << "getProperty failed for property " << static_cast<int32_t>(property); \
    UT_ASSERT_TRUE((aidl_return).has_value()) << "No value returned for property " << static_cast<int32_t>(property); \
} while (0)

#define VIDEODECODER_GET_PROPERTY_SUCCESS(videoDecoder, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        ::com::rdk::hal::State state; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", videodecoder::toString(property).c_str()); \
        status = (videoDecoder)->getProperty(property, &propertyValue); \
        switch (propertyValue->value.value().getTag()) \
        { \
            case PropertyValue::Value::Tag::intValue: \
                UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
                propertyValue->value->get<PropertyValue::Value::Tag::intValue>(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                break; \
            case PropertyValue::Value::Tag::stringValue: \
                UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%s]), status:[isOK:%s, exception:%s]", videodecoder::toString(property).c_str(), \
                propertyValue->value->get<PropertyValue::Value::Tag::stringValue>().string(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                break; \
        } \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

/**
 * @brief Calls getPropertyMulti and verifies the result.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in,out] propertyKVList The list of PropertyKVPair to query and populate.
 * @param[out] aidl_return The boolean result of the operation.
 */
#define VIDEODECODER_GETPROPERTYMULTI(videoDecoder, propertyKVList, aidl_return) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getPropertyMulti(IN:propertyKVList.size:[%zu], OUT:_aidl_return)", (propertyKVList).size()); \
        status = (videoDecoder)->getPropertyMulti(&(propertyKVList), &(aidl_return)); \
        UT_LOG_INFO("Result getPropertyMulti(OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, (aidl_return)), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "Binder call failed: " << status.exceptionMessage().c_str(); \
        UT_ASSERT_TRUE(aidl_return) << "getPropertyMulti returned false."; \
    } while (0)

/**
 * @brief Adds a PropertyKVPair to a vector with the specified property.
 * @param[in,out] propertyKVList The vector to append to.
 * @param[in] prop The Property enum value to set.
 */
#define VIDEODECODER_ADD_PROPERTYKVPAIR(propertyKVList, prop) \
    do { \
        PropertyKVPair kvPair; \
        kvPair.property = videodecoder::Property::prop; \
        kvPair.propertyValue.value = PropertyValue::Value(); \
        (propertyKVList).push_back(kvPair); \
    } while (0)

    /**
 * @brief Calls getPropertyMulti for a negative test and verifies failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in,out] propertyKVList The list of PropertyKVPair to query (invalid or empty).
 * @param[out] aidl_return The boolean result, expected to be false.
 */
#define VIDEODECODER_GETPROPERTYMULTI_FAILURE(videoDecoder, propertyKVList, aidl_return, exception) \
do { \
    Status status; \
    UT_LOG_INFO("Calling getPropertyMulti(IN:propertyKVList.size:[%zu], OUT:_aidl_return)", (propertyKVList).size()); \
    status = (videoDecoder)->getPropertyMulti(&(propertyKVList), &(aidl_return)); \
    UT_LOG_INFO("Result getPropertyMulti(OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                UT_Control_GetMapString(bool_mapTable, (aidl_return)), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
    UT_ASSERT_FALSE(aidl_return) << "getPropertyMulti should return false for invalid/empty input"; \
    UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
} while (0)

/**
 * @brief Calls getState and verifies the result.
 * @param[in] videoDecoder The IVideoDecoder instance (not IVideoDecoderController).
 * @param[out] state The returned State enum value.
 */
#define VIDEODECODER_GETSTATE(videoDecoder, state) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling getState(OUT:_aidl_return:[state])"); \
        status = (videoDecoder)->getState(&state); \
        UT_LOG_INFO("Result getState(OUT:_aidl_return:[state:%d]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(state), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "getState binder call failed"; \
        UT_ASSERT_TRUE((state) == State::UNKNOWN || \
                       (state) == State::CLOSED || \
                       (state) == State::OPENING || \
                       (state) == State::READY || \
                       (state) == State::STARTING || \
                       (state) == State::STARTED || \
                       (state) == State::FLUSHING || \
                       (state) == State::STOPPING || \
                       (state) == State::CLOSING) \
            << "Invalid state returned: " << static_cast<int32_t>(state); \
    } while (0)

#define VIDEODECODER_CHECK_STATE(videoDecoder, expected_state) \
    do { \
        State currentState; \
        Status status = (videoDecoder)->getState(&currentState); \
        UT_LOG_INFO("Calling getState(OUT:_aidl_return:[state]) for decoder"); \
        UT_LOG_INFO("Result getState(OUT:_aidl_return:[state:%d]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(currentState), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "getState binder call failed"; \
        UT_ASSERT_EQUAL(currentState, expected_state) << "Expected state " << static_cast<int32_t>(expected_state) \
                                                  << ", got " << static_cast<int32_t>(currentState); \
    } while (0)
/**
 * @brief Waits for the video decoder to reach a target state.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] targetState The expected state to wait for.
 * @param[out] lastState The final state observed.
 */
#define VIDEODECODER_WAIT_FOR_STATE(videoDecoder, targetState, lastState) \
    do { \
        int32_t sleep_time = 0; \
        do \
        { \
            std::this_thread::sleep_for(std::chrono::milliseconds(VIDEO_MAX_TRANSITORY_STATE_WAIT_MS)); \
            sleep_time += VIDEO_MAX_TRANSITORY_STATE_WAIT_MS; \
            /* Check the state of the decoder */ \
            VIDEODECODER_GETSTATE(videoDecoder, lastState); \
        } while (sleep_time < VIDEO_MAX_FINAL_STATE_WAIT_MS && lastState != targetState); \
    } while (0)

/**
 * @brief Opens the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] codec The Codec enum value to open.
 * @param[in] secure Whether to open in secure mode.
 * @param[in] listener The controller listener.
 * @param[out] controller The returned IVideoDecoderController.
 * @param[in] stateCheck Whether to validate the state after opening.
 */
#define VIDEODECODER_OPEN_SUCCESS(videoDecoder, codec, secure, listener, controller, stateCheck) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller[])", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), (listener).get()); \
        status = (videoDecoder)->open((codec), (secure), (listener), &(controller)); \
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller:[0x%X]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), (listener).get(), (controller).get(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()) << "open binder call failed"; \
        UT_ASSERT_NOT_EQUAL_FATAL((controller).get(), nullptr) << "Controller is null; codec or secure mode may not be supported"; \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE) << "open returned an exception"; \
        if (stateCheck) { \
            State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, State::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, State::READY) << "Decoder did not reach READY state after open"; \
        } \
    } while (0)

/**
 * @brief Tests opening the video decoder with expected failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] codec The Codec enum value to test.
 * @param[in] secure The secure mode to test.
 * @param[in] listener The controller listener (or nullptr).
 * @param[out] controller The returned IVideoDecoderController.
 * @param[in] exception The expected exception code (or EX_NONE for unsupported cases).
 */
#define VIDEODECODER_OPEN_FAILURE(videoDecoder, codec, secure, listener, controller, exception) \
    do { \
        Status status; \
        sp<IVideoDecoderControllerListener> tempListener = (listener); \
        void* listenerPtr = tempListener ? tempListener.get() : nullptr; \
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller[])", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), listenerPtr); \
        status = (videoDecoder)->open((codec), (secure), (listener), &(controller)); \
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%s], IN:listener:[0x%p], OUT:controller:[0x%X]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(codec), UT_Control_GetMapString(bool_mapTable, (secure)), listenerPtr, (controller).get(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if ((exception) == Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.isOk()) << "Binder call failed unexpectedly"; \
            UT_ASSERT_EQUAL((controller).get(), nullptr) << "Expected null controller for unsupported parameters"; \
        } else { \
            UT_ASSERT_FALSE(status.isOk()) << "Expected failure with exception"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)


/**
 * @brief Closes the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] controller The IVideoDecoderController instance.
 * @param[in] stateCheck Whether to validate the state after closing.
 */
#define VIDEODECODER_CLOSE_SUCCESS(videoDecoder, controller, stateCheck) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderController> tempController = (controller); \
        void* controllerPtr = tempController ? tempController.get() : nullptr; \
        UT_LOG_INFO("Calling close(IN:videoDecoderController:[0x%p], OUT:_aidl_return[])", controllerPtr); \
        status = (videoDecoder)->close((controller), &aidl_return); \
        UT_LOG_INFO("Result close(IN:videoDecoderController:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    controllerPtr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()) << "close binder call failed"; \
        UT_ASSERT_TRUE_FATAL(aidl_return) << "close returned false"; \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE) << "close returned an exception"; \
        if (stateCheck) { \
            State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, State::CLOSED, lastState); \
            UT_ASSERT_EQUAL(lastState, State::CLOSED) << "Decoder did not reach CLOSED state after close"; \
        } \
    } while (0)

/**
 * @brief Tests closing the video decoder with expected failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] controller The IVideoDecoderController instance (or nullptr).
 * @param[in] exception The expected exception code (or EX_NONE for false return).
 */
#define VIDEODECODER_CLOSE_FAILURE(videoDecoder, controller, exception) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderController> tempController = (controller); \
        void* controllerPtr = tempController ? tempController.get() : nullptr; \
        UT_LOG_INFO("Calling close(IN:videoDecoderController:[0x%p], OUT:_aidl_return[])", controllerPtr); \
        status = (videoDecoder)->close((controller), &aidl_return); \
        UT_LOG_INFO("Result close(IN:videoDecoderController:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    controllerPtr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if ((exception) == Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.isOk()) << "Binder call failed unexpectedly"; \
            UT_ASSERT_FALSE(aidl_return) << "Expected close to return false"; \
        } else { \
            UT_ASSERT_FALSE(status.isOk()) << "Expected failure with exception"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)

/**
 * @brief Calls open and verifies a valid controller is returned.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] codec The codec to use.
 * @param[in] secure The secure mode flag.
 * @param[in] listener The controller listener.
 * @param[out] controller The returned IVideoDecoderController.
 */
#define VIDEODECODER_OPEN(videoDecoder, codec, secure, listener, controller) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[])", \
                    static_cast<int32_t>(codec), (secure), (listener).get()); \
        status = (videoDecoder)->open((codec), (secure), (listener), &(controller)); \
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(codec), (secure), (listener).get(), (controller).get(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE((controller) != nullptr); \
    } while (0)

/**
 * @brief Registers an event listener with the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] listener The IVideoDecoderEventListener to register.
 */
#define VIDEODECODER_REGISTER_EVENT_SUCCESS(videoDecoder, listener) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderEventListener> tempListener = (listener); \
        void* listenerPtr = tempListener ? tempListener.get() : nullptr; \
        UT_LOG_INFO("Calling registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return[])", listenerPtr); \
        status = (videoDecoder)->registerEventListener((listener), &aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    listenerPtr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "registerEventListener binder call failed"; \
        UT_ASSERT_TRUE(aidl_return) << "registerEventListener returned false"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "registerEventListener returned an exception"; \
    } while (0)

/**
 * @brief Unregisters an event listener from the video decoder and verifies success.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] listener The IVideoDecoderEventListener to unregister.
 */
#define VIDEODECODER_UNREGISTER_EVENT_SUCCESS(videoDecoder, listener) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderEventListener> tempListener = (listener); \
        void* listenerPtr = tempListener ? tempListener.get() : nullptr; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return[])", listenerPtr); \
        status = (videoDecoder)->unregisterEventListener((listener), &aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    listenerPtr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "unregisterEventListener binder call failed"; \
        UT_ASSERT_TRUE(aidl_return) << "unregisterEventListener returned false"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "unregisterEventListener returned an exception"; \
    } while (0)

/**
 * @brief Tests registering an event listener with expected failure.
 * @param[in] videoDecoder The IVideoDecoder instance.
 * @param[in] listener The IVideoDecoderEventListener (or nullptr).
 * @param[in] exception The expected exception code (or EX_NONE for false return).
 */
#define VIDEODECODER_REGISTER_EVENT_FAILURE(videoDecoder, listener, exception) \
    do { \
        Status status; \
        bool aidl_return; \
        sp<IVideoDecoderEventListener> tempListener = (listener); \
        void* listenerPtr = tempListener ? tempListener.get() : nullptr; \
        UT_LOG_INFO("Calling registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return[])", listenerPtr); \
        status = (videoDecoder)->registerEventListener((listener), &aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener:[0x%p], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]", \
                    listenerPtr, UT_Control_GetMapString(bool_mapTable, aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if ((exception) == Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.isOk()) << "Binder call failed unexpectedly"; \
            UT_ASSERT_FALSE(aidl_return) << "Expected registerEventListener to return false"; \
        } else { \
            UT_ASSERT_FALSE(status.isOk()) << "Expected failure with exception"; \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)

#define VIDEODECODER_UNREGISTER_EVENT_FAILURE(videoDecoder, eventListener, exception) \
    do { \
        Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[])", (eventListener) ? (eventListener).get() : nullptr); \
        status = (videoDecoder)->unregisterEventListener((eventListener), &_aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[0x%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
                    (eventListener) ? (eventListener).get() : nullptr, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if ((exception) == Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.isOk()); \
            UT_ASSERT_FALSE(_aidl_return) << "Expected unregisterEventListener to return false"; \
        } else { \
            UT_ASSERT_FALSE(status.isOk()); \
            UT_ASSERT_EQUAL(status.exceptionCode(), (exception)) << "Unexpected exception code"; \
        } \
    } while (0)

#define VIDEODECODER_START_SUCCESS(videoDecoder, videoDecoderController, stateCheck) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling start()"); \
        status = (videoDecoderController)->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "start binder call failed"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "start returned an exception"; \
        if (stateCheck) { \
            State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, State::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, State::STARTED) << "Decoder did not reach STARTED state"; \
        } \
    } while (0)

#define VIDEODECODER_STOP_SUCCESS(videoDecoder, videoDecoderController, stateCheck) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling stop()"); \
        status = videoDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) { \
            State lastState; \
            VIDEODECODER_WAIT_FOR_STATE(videoDecoder, State::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, State::READY) << "Decoder did not reach READY state"; \
        } \
    } while (0)

#define VIDEODECODER_START_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling start() expecting failure"); \
        status = videoDecoderController->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), expectedException); \
    } while (0)

#define VIDEODECODER_STOP_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling stop() expecting failure"); \
        status = videoDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), expectedException); \
    } while (0)

/**
 * @brief Calls setProperty on an IVideoDecoder instance to set an integer property value.
 * @param[in] videoDecoder The IVideoDecoder instance (not IVideoDecoderController).
 * @param[in] property The Property enum key (must be read-write, e.g., OPERATIONAL_MODE).
 * @param[in] value The integer value to set.
 * @param[in] value_desc String description of the value for logging.
 */

#define VIDEODECODER_SETPROPERTY_SUCCESS(videoDecoderController, property, propertyValue) \
    do { \
        Status status; \
        bool result; \
        UT_LOG_INFO("Calling setProperty(IN:property[%d], IN:propertyValue=%d)", \
                    static_cast<int32_t>(property), propertyValue.value); \
        status = videoDecoderController->setProperty(property, propertyValue, &result); \
        UT_LOG_INFO("Result setProperty(IN:property[%d], OUT:result=%s), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(property), UT_Control_GetMapString(bool_mapTable, result), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(result); \
    } while (0)

#define VIDEODECODER_SETPROPERTY_FAILURE(videoDecoderController, property, propertyValue, expectedException) \
    do { \
        Status status; \
        bool result; \
        UT_LOG_INFO("Calling setProperty(IN:property[%d], IN:propertyValue=%d) expecting failure", \
                    static_cast<int32_t>(property), \
                    propertyValue.value); \
        status = videoDecoderController->setProperty(property, propertyValue, &result); \
        UT_LOG_INFO("Result setProperty(IN:property[%d], OUT:result=%d), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(property), result, \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), expectedException); \
    } while (0)

    #define VIDEODECODER_SETPROPERTY_LIST_SUCCESS(videoDecoderController, propertyList, type) \
    do { \
        for (const auto& [property, value] : propertyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            VIDEODECODER_SETPROPERTY_SUCCESS(videoDecoderController, property, propertyValue); \
        } \
    } while (0)

#define VIDEODECODER_SETPROPERTY_LIST_FAILURE(videoDecoderController, propertyList, type, exception) \
    do { \
        for (const auto& [property, value] : propertyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            VIDEODECODER_SETPROPERTY_FAILURE(videoDecoderController, property, propertyValue, exception); \
        } \
    } while (0)

#define VIDEODECODER_CONTROLLER_SET_PROPERTY(videoDecoderController, property, propertyValue, status, result) \
    do { \
        UT_LOG_INFO("Calling setProperty(IN:property[%d], IN:propertyValue=%d)", \
                    static_cast<int32_t>(property), \
                    propertyValue.value.has_value() ? propertyValue.value->intValue : 0); \
        status = videoDecoderController->setProperty(property, propertyValue, &result); \
        UT_LOG_INFO("Result setProperty(IN:property[%d], OUT:result=%d), status:[isOK:%s, exception:%s]", \
                    static_cast<int32_t>(property), result, \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
    } while (0)

/**
 * @brief Tests that flush fails with the expected exception in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] reset The reset flag for flush.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_FLUSH_FAILURE(videoDecoderController, reset, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling flush(IN:reset:[%s])", (reset) ? "true" : "false"); \
        status = (videoDecoderController)->flush(reset); \
        UT_LOG_INFO("Result flush(IN:reset:[%s]), status:[isOK:%s, exception:%s]", \
                    (reset) ? "true" : "false", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "flush succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for flush"; \
    } while (0)
/**
 * @brief Tests that flush fails with the expected exception in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] reset The reset flag for flush.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */

#define VIDEODECODER_FLUSH_SUCCESS(videoDecoder, videoDecoderController, reset, stateCheck) \
    do { \
        State lastState; \
        Status status = (videoDecoderController)->flush(reset); \
        UT_LOG_INFO("flush(%s) status=%s", reset ? "true" : "false", UT_Control_GetMapString(bool_mapTable, status.isOk())); \
        UT_ASSERT_TRUE(status.isOk()) << "flush(" << reset << ") failed"; \
        VIDEODECODER_WAIT_FOR_STATE(videoDecoder, State::READY, lastState); \
        if (stateCheck) { \
            State lastState; \
            if (reset){ \
                VIDEODECODER_WAIT_FOR_STATE(videoDecoder, State::READY, lastState); \
                UT_ASSERT_EQUAL(lastState, State::READY) << "Decoder did not reach READY state"; \
            } \
            else { \
                VIDEODECODER_WAIT_FOR_STATE(videoDecoder, State::STARTED, lastState); \
                UT_ASSERT_EQUAL(lastState, State::STARTED) << "Decoder did not reach STARTED state"; \
            } \
        } \
    } while (0)
/**
 * @brief Tests that signalDiscontinuity succeeds in the STARTED state.
 * @param[in] videoDecoder The IVideoDecoder instance (for state checking).
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] expectSuccess Expect the call to succeed (true for positive test).
 */
#define VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(videoDecoder, videoDecoderController, expectSuccess) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = (videoDecoderController)->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk() == (expectSuccess)) << "signalDiscontinuity " \
            << ((expectSuccess) ? "failed unexpectedly" : "succeeded unexpectedly"); \
    } while (0)

/**
 * @brief Tests that signalDiscontinuity fails with the expected exception in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_SIGNAL_DISCONTINUITY_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = (videoDecoderController)->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "signalDiscontinuity succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for signalDiscontinuity"; \
    } while (0)

/**
 * @brief Tests that signalEOS succeeds in the STARTED state.
 * @param[in] videoDecoder The IVideoDecoder instance (for state checking).
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] expectSuccess Expect the call to succeed (true for positive test).
 */
#define VIDEODECODER_SIGNAL_EOS_SUCCESS(videoDecoder, videoDecoderController, expectSuccess) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalEOS()"); \
        status = (videoDecoderController)->signalEOS(); \
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk() == (expectSuccess)) << "signalEOS " \
            << ((expectSuccess) ? "failed unexpectedly" : "succeeded unexpectedly"); \
    } while (0)

/**
 * @brief Tests that signalEOS fails with the expected exception in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_SIGNAL_EOS_FAILURE(videoDecoderController, expectedException) \
    do { \
        Status status; \
        UT_LOG_INFO("Calling signalEOS()"); \
        status = (videoDecoderController)->signalEOS(); \
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "signalEOS succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for signalEOS"; \
    } while (0)

/**
 * @brief Tests that parseCodecSpecificData succeeds in STARTED with valid data.
 * @param[in] videoDecoder The IVideoDecoder instance (for state checking).
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] csdVideoFormat The codec-specific data format.
 * @param[in] codecData The codec data byte array.
 * @param[in] expectSuccess Expect the call to succeed (true for positive test).
 */

#define VIDEODECODER_PARSE_CSD_SUCCESS(videoDecoder, videoDecoderController, csdVideoFormat, codecData, expectSuccess) \
    do { \
        bool result = false; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[%d], IN:codecData:size:[%zu])", \
                    static_cast<int32_t>(csdVideoFormat), codecData.size()); \
        Status status = (videoDecoderController)->parseCodecSpecificData(csdVideoFormat, codecData, &result); \
        UT_LOG_INFO("Result parseCodecSpecificData(), status:[isOK:%s, exception:%s], result:[%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), \
                    UT_Control_GetMapString(bool_mapTable, result)); \
        UT_ASSERT_TRUE(status.isOk() == (expectSuccess)) << "parseCodecSpecificData status " \
            << ((expectSuccess) ? "failed unexpectedly" : "succeeded unexpectedly"); \
        if (expectSuccess) { \
            UT_ASSERT_TRUE(result) << "parseCodecSpecificData returned false"; \
        } \
    } while (0)

/**
 * @brief Tests that parseCodecSpecificData fails with EX_ILLEGAL_STATE in an invalid state.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] csdVideoFormat The codec-specific data format.
 * @param[in] codecData The codec data byte array.
 * @param[in] expectedException The expected binder exception (e.g., EX_ILLEGAL_STATE).
 */
#define VIDEODECODER_PARSE_CSD_STATE_FAILURE(videoDecoderController, csdVideoFormat, codecData, expectedException) \
    do { \
        bool result = false; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[%d], IN:codecData:size:[%zu])", \
                    static_cast<int32_t>(csdVideoFormat), codecData.size()); \
        Status status = (videoDecoderController)->parseCodecSpecificData(csdVideoFormat, codecData, &result); \
        UT_LOG_INFO("Result parseCodecSpecificData(), status:[isOK:%s, exception:%s], result:[%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), \
                    UT_Control_GetMapString(bool_mapTable, result)); \
        UT_ASSERT_FALSE(status.isOk()) << "parseCodecSpecificData succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for parseCodecSpecificData"; \
    } while (0)

/**
 * @brief Tests that parseCodecSpecificData fails with false return for invalid data.
 * @param[in] videoDecoderController The IVideoDecoderController instance.
 * @param[in] csdVideoFormat The codec-specific data format.
 * @param[in] codecData The invalid codec data byte array.
 */
#define VIDEODECODER_PARSE_CSD_DATA_FAILURE(videoDecoderController, csdVideoFormat, codecData, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[%d], IN:codecData:size:[%zu])", \
                    static_cast<int32_t>(csdVideoFormat), codecData.size()); \
        status = videoDecoderController->parseCodecSpecificData(csdVideoFormat, codecData, &_aidl_return); \
        UT_LOG_INFO("Result parseCodecSpecificData(), status:[isOK:%s, exception:%s], result:[%s]", \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), \
                    UT_Control_GetMapString(bool_mapTable, _aidl_return)); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define VIDEODECODER_DECODEBUFFER_SUCCESS(controller, pts, bufferHandle) \
    do { \
        bool decodeResult; \
        Status status = controller->decodeBuffer(pts, bufferHandle, &decodeResult); \
        UT_LOG_INFO("decodeBuffer(pts=%lld, bufferHandle=0x%016"PRIx64") status=%s, result=%s", \
                    static_cast<long long>(pts), static_cast<long long>(bufferHandle), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    decodeResult ? "true" : "false"); \
        UT_ASSERT_TRUE(status.isOk()) << "decodeBuffer failed"; \
        UT_ASSERT_TRUE(decodeResult) << "decodeBuffer returned false"; \
    } while (0)

#define VIDEODECODER_DECODEBUFFER_FAILURE(controller, pts, bufferHandle, expectedException) \
    do { \
        bool decodeResult; \
        Status status = controller->decodeBuffer(pts, bufferHandle, &decodeResult); \
        UT_LOG_INFO("decodeBuffer(pts=%lld, bufferHandle=0x%016"PRIx64") status=%s, result=%s", \
                    static_cast<long long>(pts), static_cast<long long>(bufferHandle), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    decodeResult ? "true" : "false"); \
        UT_ASSERT_FALSE(status.isOk() || decodeResult) << "decodeBuffer unexpectedly succeeded"; \
        if (expectedException != Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.exceptionCode() == expectedException) \
                << "decodeBuffer returned unexpected exception"; \
        } \
    } while (0)

/**
 * @brief Test fixture for Level 1 Video Decoder HALIF tests.
 * Inherits from UTCore to leverage ut-core's test utilities and lifecycle methods.
 */
class VTS_L1_VideoDecoder : public UTCore {
    public:
        /**
         * @brief Default constructor.
         * Relies on UTCore's default initialization; no additional setup required.
         */
        VTS_L1_VideoDecoder() = default;

        /**
         * @brief Virtual destructor.
         * Ensures proper cleanup; defaults to UTCore's implementation.
         */
        ~VTS_L1_VideoDecoder() override = default;

        /**
         * @brief Prepares the test environment before each test case.
         * Calls UTCore::SetUp() to ensure base class initialization.
         * Add custom setup logic here if needed.
         */
        void SetUp() override {
            if (gVideoDecoderManager == nullptr) {
                UT_FAIL_FATAL("VideoDecoder Service not available");
            }

            UTCore::SetUp();  // Call base class setup (if it exists)
            // Custom setup code here, e.g., initialize decoder mocks or profiles
        }

        /**
         * @brief Cleans up the test environment after each test case.
         * Calls UTCore::TearDown() to ensure base class cleanup.
         * Add custom cleanup logic here if needed.
         */
        void TearDown() override {
            // Custom cleanup code here, e.g., release resources
            UTCore::TearDown();  // Call base class teardown (if it exists)
        }
    };

class VideoDecoderControllerListener : public BnVideoDecoderControllerListener {
public:
    VideoDecoderControllerListener() {}
    virtual ~VideoDecoderControllerListener() = default;

    Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<com::rdk::hal::videodecoder::FrameMetadata>& metadata) override {
        UT_LOG_INFO("onFrameOutput received: nsPresentationTime=%lld, frameBufferHandle=0x%016"PRIx64"", nsPresentationTime, frameBufferHandle);
        DecodedVideoFrame_t frame;
        frame.framePTS = nsPresentationTime;
        frame.receiveTime = std::chrono::steady_clock::now();
        frame.isMetaAvailable = metadata.has_value();
        frame.frameBufferHandle = frameBufferHandle; // Store handle
        if (metadata.has_value()) {
            frame.metadata = std::make_unique<com::rdk::hal::videodecoder::FrameMetadata>(metadata.value());

            UT_LOG_INFO("FrameMetadata: codedWidth=%d, codedHeight=%d, activeX=%d, activeY=%d, activeWidth=%d, activeHeight=%d",
                        frame.metadata->codedWidth, frame.metadata->codedHeight,
                        frame.metadata->activeX, frame.metadata->activeY,
                        frame.metadata->activeWidth, frame.metadata->activeHeight);

            UT_LOG_INFO("AspectRatios: PAR=%d:%d, SAR=%d:%d",
                        frame.metadata->parX, frame.metadata->parY,
                        frame.metadata->sarX, frame.metadata->sarY);

            UT_LOG_INFO("FrameMetadata: colorDepth=%d, pixelFormat=%d, dynamicRange=%d, scanType=%d, afd=%d",
                        frame.metadata->colorDepth,
                        static_cast<int>(frame.metadata->pixelFormat),
                        static_cast<int>(frame.metadata->dynamicRange),
                        static_cast<int>(frame.metadata->scanType),
                        frame.metadata->afd);

            UT_LOG_INFO("FrameRate: %d/%d",
                        frame.metadata->frameRateNumerator,
                        frame.metadata->frameRateDenominator);

            UT_LOG_INFO("Flags: endOfStream=%d, discontinuity=%d, lowLatency=%d",
                        frame.metadata->endOfStream,
                        frame.metadata->discontinuity,
                        frame.metadata->lowLatency);

            UT_LOG_INFO("Source: %d", static_cast<int>(frame.metadata->source));

        } else {
            UT_LOG_INFO("No FrameMetadata provided");
        }


        {
            std::lock_guard<std::mutex> lock(mMutex);
            mDecodedFrames.push_back(std::move(frame)); // Move is now possible
        }
        mCV.notify_all();
        return Status::ok();
    }

    Status onUserDataOutput(int64_t nsPresentationTime, const ::std::vector<uint8_t>& userData) override {
        UT_LOG_INFO("onUserDataOutput received");
        return Status::ok();
    }

    bool getNextFrame(DecodedVideoFrame_t& frame, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mCV.wait_for(lock, timeout, [this]() { return !mDecodedFrames.empty(); })) {
            frame = std::move(mDecodedFrames.front()); // Move is now possible
            mDecodedFrames.pop_front();
            return true;
        }
        return false;
    }

private:
    std::mutex mMutex;
    std::condition_variable mCV;
    std::deque<DecodedVideoFrame_t> mDecodedFrames;
};

class VideoDecoderEventListener : public BnVideoDecoderEventListener {
    public:
    VideoDecoderEventListener() {};
    virtual ~VideoDecoderEventListener() = default;

    Status onDecodeError(videodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override {
        UT_LOG_ERROR("Decode error: errorCode=%d, vendorErrorCode=%d", static_cast<int32_t>(errorCode), vendorErrorCode);
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mErrors.push_back(static_cast<int32_t>(errorCode));
        }
        mCV.notify_all();
        return Status::ok();
    }
    Status onStateChanged(State oldState,State newState) override
    {
        UT_LOG("State Change received: %s -> %s", com::rdk::hal::toString(oldState).c_str(), com::rdk::hal::toString(newState).c_str());
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStateChanges.emplace_back(oldState, newState);
            mCurrentState = newState;
        }
        mCV.notify_all(); // Unblock any waiting test code
        return Status::ok();
    }
    bool waitForError(videodecoder::ErrorCode desiredError, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mMutex);
        return mCV.wait_for(lock, timeout, [this, desiredError]() {
            return std::find(mErrors.begin(), mErrors.end(), static_cast<int32_t>(desiredError)) != mErrors.end();
        });
    }
    /**
     * Wait for the sink to reach a particular State within a given timeout.
     *
     * @param desiredState The state we want to observe.
     * @param timeout      How long to wait, e.g. std::chrono::milliseconds(500).
     * @return true if the state was reached; false if we timed out waiting.
     */
    bool waitForState(State desiredState, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        // Wait until mCurrentState == desiredState or until timeout
        return mCV.wait_for(lock, timeout, [this, desiredState]() {
            return (mCurrentState == desiredState);
        });
    }
    // Accessors for test verifications if desired
    std::vector<std::pair<State, State>> getStateChanges() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mStateChanges;
    }

    void resetStateChanges()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStateChanges.clear();
    }

    bool checkStateTransistion(State expOldState, State expNewState)
    {
        bool foundClosedToOpening = false;
        for (const auto &[oldState, newState] : mStateChanges)
        {
            if (oldState == expOldState && newState == expNewState)
            {
                foundClosedToOpening = true;
                break;
            }
        }
        return foundClosedToOpening;
    }

    std::vector<int32_t> getErrors() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mErrors;
    }
    private:
        mutable std::mutex mMutex;
        std::condition_variable mCV;

        State mCurrentState;
        std::vector<std::pair<State, State>> mStateChanges;
        std::vector<int32_t> mErrors;
};

// AVBufferSpaceListener declaration
class AVBufferSpaceListener : public BnAVBufferSpaceListener {
public:
    AVBufferSpaceListener() {}
    virtual ~AVBufferSpaceListener() = default;
    Status onSpaceAvailable() override;
};

// DecoderTest declaration
class videoDecoderTest {
public:
    videoDecoderTest(IVideoDecoder::Id decoderId, sp<IVideoDecoder> decoderHandle,
                sp<VideoDecoderControllerListener> controlListener,
                sp<VideoDecoderEventListener> eventListener,
                sp<IAVBuffer> AVBufferManager,
                bool secureDecode, bool tunnelMode, videoDecoderTestConfiguration_t config,IAVBufferHelper* avbufferHelper){
                    mDecoderId = decoderId;
                    mDecoderHandle = decoderHandle;
                    mControlListener = controlListener;
                    mSecureDecode = secureDecode;
                    mEventListener = eventListener;
                    mTestConfiguration = config;
                    mAVBufferManager = AVBufferManager;
                    mAVbufferHelper = avbufferHelper;
                }
    void run(bool concurrentDecodeConsume);
    bool getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped);
    std::vector<std::tuple<uint64_t, uint32_t, int64_t>> parseFrames(
        const std::vector<uint8_t>& inputBuffer, uint64_t fileSize, videodecoder::Codec codec);

private:
    thread mDecoderThread, mConsumerThread;
    IVideoDecoder::Id mDecoderId;
    sp<IVideoDecoder> mDecoderHandle;
    sp<VideoDecoderControllerListener> mControlListener;
    sp<VideoDecoderEventListener> mEventListener;
    sp<IAVBuffer> mAVBufferManager;
    IAVBufferHelper* mAVbufferHelper;
    bool mSecureDecode;
    bool mTunnelMode;
    videoDecoderTestConfiguration_t mTestConfiguration;
    deque<InputVideoFrame_t> mInputFrame;
    mutex mDecodeMutex;
    atomic<bool> mDecodingStream;
    bool mConcurrentDecodeConsume;
    mutex mAVbufferHelperLock;

    void decoderThreadFunc();
    void consumerThreadFunc();
    void decodeAndConsumeVideoStreams();
};


#endif //__VTS_VIDEODECODER_H__


/** @} */ // End of Video_Decoder_Halif_Test_Header
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK
