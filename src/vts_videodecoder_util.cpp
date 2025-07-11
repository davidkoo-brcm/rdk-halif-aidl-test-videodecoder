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
 * @defgroup Video_Decoder_Halif_Test_Util Video Decoder HALIF Tests util File
 * @{
 * @parblock
 *
 * ### utility functions for Video Decoder VTS HALIF AIDL tests :
 *
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
 * @file vts_videodecoder_util.cpp
 *
 */

#include "vts_videodecoder.h"

using namespace std;

extern sp<IVideoDecoderManager> gVideoDecoderManager;
std::vector<decoder_t> gVideodecoders_hfp;

UT_status_t getVideoDecoderService() {
    gVideoDecoderManager = android::interface_cast<IVideoDecoderManager>(
       defaultServiceManager()->getService(String16(IVideoDecoderManager::serviceName().c_str())));
   if (gVideoDecoderManager == nullptr) {
       UT_LOG_ERROR("Video Decoder Service is not available");
       return UT_STATUS_FAILURE;
   }
   return UT_STATUS_OK;
}

sp<IAVBuffer> gAVBufferManager;

sp<IAVBuffer> getAVBufferService()
{
    return
        android::interface_cast<IAVBuffer>(
            android::defaultServiceManager()->getService(
                android::String16(IAVBuffer::serviceName().c_str())
            )
        );
}

Codec videoDecoderStringToCodec(const std::string& codecStr) {
    static const std::unordered_map<std::string, Codec> codecMap = {
        {"MPEG", Codec::MPEG},
        {"H264", Codec::H264},
        {"H265", Codec::H265},
        {"VP9",  Codec::VP9},
        {"AV1",  Codec::AV1}
    };
    auto it = codecMap.find(codecStr);
    if (it != codecMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Unknown codec: " + codecStr);
    }
}

DynamicRange videoDecoderStringToDynamicRange(const std::string& rangeStr) {
    static const std::unordered_map<std::string, DynamicRange> dynamicRangeMap = {
        {"UNKNOWN", DynamicRange::UNKNOWN},
        {"SDR", DynamicRange::SDR},
        {"HLG", DynamicRange::HLG},
        {"HDR10", DynamicRange::HDR10},
        {"HDR10PLUS", DynamicRange::HDR10PLUS},
        {"DOLBY_VISION", DynamicRange::DOLBY_VISION}
    };
    auto it = dynamicRangeMap.find(rangeStr);
    return (it != dynamicRangeMap.end()) ? it->second : DynamicRange::UNKNOWN;
}

ScanType videoDecoderStringToScanType(const std::string& scanTypeStr) {
    static const std::unordered_map<std::string, ScanType> scanTypeMap = {
        {"PROGRESSIVE_FRAME", ScanType::PROGRESSIVE_FRAME},
        {"INTERLACED_FRAME", ScanType::INTERLACED_FRAME}
    };
    auto it = scanTypeMap.find(scanTypeStr);
    return (it != scanTypeMap.end()) ? it->second : ScanType::PROGRESSIVE_FRAME; // Default fallback
}

OperationalMode videoDecoderStringToOperationalMode(const std::string& modeStr) {
    static const std::unordered_map<std::string, OperationalMode> modeMap = {
        {"TUNNELLED", OperationalMode::TUNNELLED},
        {"NON_TUNNELLED", OperationalMode::NON_TUNNELLED},
        {"GRAPHICS_TEXTURE", OperationalMode::GRAPHICS_TEXTURE}
    };

    auto it = modeMap.find(modeStr);
    if (it != modeMap.end()) {
        return it->second;  // Return the integer value instead of enum
    } else {
        return OperationalMode::TUNNELLED;  // Return TUNNELLED for unknown/default modes
    }
}

PixelFormat videoDecoderStringToPixelFormat(const std::string& formatStr) {
    static const std::unordered_map<std::string, PixelFormat> pixelFormatMap = {
        {"YCBCR422", PixelFormat::YCBCR422},
        {"YCBCR444", PixelFormat::YCBCR444},
        {"YCBCR420", PixelFormat::YCBCR420},
        {"RGB", PixelFormat::RGB},
        {"NATIVE", PixelFormat::NATIVE}
    };
    auto it = pixelFormatMap.find(formatStr);
    if (it != pixelFormatMap.end()) {
        return it->second;
    }
    return static_cast<PixelFormat>(0); // Unknown format
}

bool vts_videodecoder_helper_validatePropertyValue(int decoder_id,const std::optional<PropertyValue>& propertyValue, const Property property) {
    // Check if the property value is present and contains a valid value
    if (!propertyValue.has_value() || !propertyValue->value.has_value()) {
        return false;
    }

    // Extract the value into a variant (only int32_t is expected per Property.aidl)
    std::variant<int32_t> val;
    const auto& propVal = propertyValue->value.value();
    switch (propVal.getTag()) {
        case PropertyValue::Value::Tag::intValue:
            val = propVal.get<PropertyValue::Value::Tag::intValue>();
            break;
        default:
            std::cerr << "Unsupported type in PropertyValue::Value for video decoder property" << std::endl;
            return false;  // All properties should be intValue per AIDL
    }

    // Validate based on the specific property
    switch (property) {
        case Property::RESOURCE_ID:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) == decoder_id;
        case Property::INPUT_QUEUE_DEPTH:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= 0;  // Non-negative size
        case Property::OUTPUT_FRAME_POOL_SIZE:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= 0;  // Non-negative size
        case Property::OPERATIONAL_MODE:
            return std::holds_alternative<int32_t>(val) &&(std::get<int32_t>(val) == static_cast<int32_t>(OperationalMode::TUNNELLED) ||
                std::get<int32_t>(val) == static_cast<int32_t>(OperationalMode::NON_TUNNELLED) ||
                std::get<int32_t>(val) == static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE) ||
                // Check for valid OR combinations (GRAPHICS_TEXTURE with TUNNELLED or NON_TUNNELLED)
                std::get<int32_t>(val) == (static_cast<int32_t>(OperationalMode::TUNNELLED) |
                static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE)) ||
                std::get<int32_t>(val) == (static_cast<int32_t>(OperationalMode::NON_TUNNELLED) |
                static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE)));
        case Property::LOW_LATENCY_MODE:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case Property::DECODE_ERROR_POLICY:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case Property::AV_SOURCE:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= 0;  // Assuming AVSource enum starts at 0
        case Property::SHA1_CALC:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case Property::SECURE_VIDEO:
            return std::holds_alternative<int32_t>(val) && (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);
        case Property::METRIC_FRAMES_DECODED:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;  // -1 for not implemented
        case Property::METRIC_DECODE_ERRORS:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;  // -1 for not implemented
        case Property::METRIC_FRAMES_DROPPED:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;  // -1 for not implemented
        default:
            return true;  // Default to true for unhandled properties (future-proofing)
    }
}


UT_status_t vts_videodecoder_parseVideoDecoderHFP()
{
    std::string videodecoderpath = VIDEODECODERMGR_IVIDEODECODER;
    std::string opmodepath = std::string(VIDEODECODERMGR) + ".supportedOperationalModes";
    int32_t number_of_videodecoders;
    int32_t supportedcodec_count;
    int32_t supporteddr_count;
    int32_t supportedopmode_count;
    decoder temporary;

    //UT_LOG_INFO("Parsing HFP from: %s", videodecoderpath.c_str());

    number_of_videodecoders = UT_KVP_PROFILE_GET_LIST_COUNT(videodecoderpath.c_str());
    //UT_LOG_INFO("Number of video decoders: %d", number_of_videodecoders);
    if (number_of_videodecoders <= 0)
    {
        UT_LOG_ERROR("No videodecoders found in HFP at path: %s", videodecoderpath.c_str());
    }

    supportedopmode_count = UT_KVP_PROFILE_GET_LIST_COUNT(opmodepath.c_str());
    //UT_LOG_INFO("Operational modes count: %d", supportedopmode_count);
    temporary.supportedOperationalModes.clear();
    for (int32_t j = 0; j < supportedopmode_count; ++j)
    {
        char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
        std::string modepath = opmodepath + "." + std::to_string(j);
        UT_KVP_PROFILE_GET_STRING(modepath.c_str(), result_kvp);
        std::string modeStr = std::string(result_kvp);
        //UT_LOG_INFO("Reading opmode path: %s, value: %s", modepath.c_str(), modeStr.c_str());
        if (!modeStr.empty()) {
            try {
                temporary.supportedOperationalModes.push_back(videoDecoderStringToOperationalMode(modeStr));
            } catch (const std::invalid_argument& e) {
                UT_LOG_WARNING("Invalid opmode at path: %s, value: %s, error: %s", modepath.c_str(), modeStr.c_str(), e.what());
            }
        } else {
            UT_LOG_WARNING("Empty opmode at path: %s", modepath.c_str());
        }
    }
    //UT_LOG_INFO("Parsed %zu operational modes", temporary.supportedOperationalModes.size());

    const std::vector<std::string> known_codecs = {"MPEG", "H264", "H265", "VP9", "AV1"};

    gVideodecoders_hfp.clear();
    for (int32_t i = 0; i < number_of_videodecoders; ++i)
    {
        std::string decoderbasepath = videodecoderpath + "." + std::to_string(i);
        std::string id = decoderbasepath + ".ID";
        std::string supportedcodecpath = decoderbasepath + ".supportedCodecs";
        std::string supporteddrpath = decoderbasepath + ".supportedDynamicRanges";
        std::string supportsSecurepath = decoderbasepath + ".supportsSecure";

        temporary.id = UT_KVP_PROFILE_GET_UINT32((decoderbasepath + ".ID").c_str());
        temporary.supportsSecure = false;
        temporary.supportedCodecs.clear();
        temporary.supportedDynamicRanges.clear();

        temporary.supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
        //UT_LOG_INFO("Decoder %d: id=%d, supportsSecure=%d", i, temporary.id, temporary.supportsSecure);

        supportedcodec_count = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcodecpath.c_str());
        //UT_LOG_INFO("Decoder %d: codec count=%d", i, supportedcodec_count);
        for (int32_t j = 0; j < supportedcodec_count; ++j)
        {
            std::string codecpath = supportedcodecpath + "." + std::to_string(j);
            bool codec_found = false;
            CodecCapability cap;

            for (const auto& codec_name : known_codecs)
            {
                std::string attrpath = codecpath + "." + codec_name;
                std::string frameratepath = attrpath + ".maxFrameRate";
                uint32_t test_rate = UT_KVP_PROFILE_GET_UINT32(frameratepath.c_str());
                if (test_rate > 0)
                {
                    //UT_LOG_INFO("Found codec at path: %s, codec: %s", codecpath.c_str(), codec_name.c_str());
                    try {
                        cap.codec = videoDecoderStringToCodec(codec_name);
                    } catch (const std::invalid_argument& e) {
                        UT_LOG_WARNING("Invalid codec: %s, error: %s, skipping", codec_name.c_str(), e.what());
                        continue;
                    }

                    cap.maxFrameRate = test_rate;
                    cap.maxFrameWidth = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameWidth").c_str());
                    cap.maxFrameHeight = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameHeight").c_str());
                    //UT_LOG_INFO("Codec %s: rate=%d, width=%d, height=%d",
                    //             codec_name.c_str(), cap.maxFrameRate, cap.maxFrameWidth, cap.maxFrameHeight);

                    if (cap.maxFrameWidth == 0 || cap.maxFrameHeight == 0) {
                        UT_LOG_WARNING("Invalid codec attributes for %s at path: %s, skipping",
                                       codec_name.c_str(), codecpath.c_str());
                        continue;
                    }

                    temporary.supportedCodecs.push_back(cap);
                    codec_found = true;
                    break;
                }
            }

            if (!codec_found) {
                UT_LOG_WARNING("No valid codec found at path: %s, skipping", codecpath.c_str());
            }
        }

        supporteddr_count = UT_KVP_PROFILE_GET_LIST_COUNT(supporteddrpath.c_str());
        //UT_LOG_INFO("Decoder %d: dynamic range count=%d", i, supporteddr_count);
        for (int32_t j = 0; j < supporteddr_count; ++j)
        {
            char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
            std::string drpath = supporteddrpath + "." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(drpath.c_str(), result_kvp);
            std::string drStr = std::string(result_kvp);
            //UT_LOG_INFO("Reading dynamic range path: %s, value: %s", drpath.c_str(), drStr.c_str());
            if (!drStr.empty()) {
                try {
                    temporary.supportedDynamicRanges.push_back(videoDecoderStringToDynamicRange(drStr));
                } catch (const std::invalid_argument& e) {
                    UT_LOG_WARNING("Invalid dynamic range at path: %s, value: %s, error: %s",
                                   drpath.c_str(), drStr.c_str(), e.what());
                }
            } else {
                UT_LOG_WARNING("Empty dynamic range at path: %s", drpath.c_str());
            }
        }

        //UT_LOG_INFO("Decoder %d: id=%d, %zu codecs, %zu dynamic ranges, %zu opmodes",
        //            i, temporary.id, temporary.supportedCodecs.size(),
        //            temporary.supportedDynamicRanges.size(), temporary.supportedOperationalModes.size());

        if (temporary.supportedCodecs.empty()) {
            UT_LOG_WARNING("Decoder %d has no valid codecs, skipping", i);
            continue;
        }
        if (temporary.supportedDynamicRanges.empty()) {
            UT_LOG_WARNING("Decoder %d has no valid dynamic ranges, skipping", i);
            continue;
        }

        gVideodecoders_hfp.push_back(temporary);
    }

    if (gVideodecoders_hfp.empty()) {
        UT_LOG_ERROR("No valid decoders parsed from HFP");
    }

    //UT_LOG_INFO("Successfully parsed %zu decoders", gVideodecoders_hfp.size());
    return UT_STATUS_OK;
}

size_t vts_bytesPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::YCBCR420: return 1.5; // Y + 0.25*Cb + 0.25*Cr
        case PixelFormat::YCBCR422: return 2;   // Y + 0.5*Cb + 0.5*Cr
        case PixelFormat::YCBCR444: return 3;   // Y + Cb + Cr
        case PixelFormat::RGB: return 3;        // R + G + B
        case PixelFormat::NATIVE: return 3;     // Assume 3 bytes
        default: return 1;
    }
}

double vts_computeVideoRMSDiff(const uint8_t* ref, const uint8_t* dec, size_t width, size_t height, PixelFormat format) {
    double sum = 0.0;
    size_t componentCount = 0;

    if (format == PixelFormat::RGB || format == PixelFormat::NATIVE) {
        // RGB or NATIVE: 3 bytes per pixel (R, G, B)
        size_t pixelCount = width * height;
        size_t byteCount = pixelCount * 3;
        for (size_t i = 0; i < byteCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
    } else if (format == PixelFormat::YCBCR444) {
        // YCBCR444: 3 bytes per pixel (Y, Cb, Cr)
        size_t pixelCount = width * height;
        size_t byteCount = pixelCount * 3;
        for (size_t i = 0; i < byteCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
    } else if (format == PixelFormat::YCBCR422) {
        // YCBCR422: 4:2:2, 2 bytes per pixel (Y0, Y1, Cb, Cr for 2 pixels)
        size_t pixelCount = width * height;
        size_t yCount = pixelCount;
        size_t chromaCount = pixelCount / 2; // One Cb/Cr per 2 Y
        size_t byteOffset = 0;
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; x += 2) {
                // Y0
                double diffY0 = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffY0 * diffY0;
                componentCount++;
                byteOffset++;
                // Cb
                double diffCb = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffCb * diffCb;
                componentCount++;
                byteOffset++;
                // Y1
                double diffY1 = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffY1 * diffY1;
                componentCount++;
                byteOffset++;
                // Cr
                double diffCr = static_cast<double>(ref[byteOffset]) - static_cast<double>(dec[byteOffset]);
                sum += diffCr * diffCr;
                componentCount++;
                byteOffset++;
            }
        }
    } else if (format == PixelFormat::YCBCR420) {
        // YCBCR420: 4:2:0, 1.5 bytes per pixel (Y for each pixel, Cb/Cr for 2x2 block)
        size_t pixelCount = width * height;
        size_t yCount = pixelCount;
        size_t chromaCount = pixelCount / 4; // One Cb/Cr per 4 Y
        // Y plane
        for (size_t i = 0; i < yCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
        // Cb plane
        size_t cbOffset = yCount;
        for (size_t i = 0; i < chromaCount; ++i) {
            double diff = static_cast<double>(ref[cbOffset + i]) - static_cast<double>(dec[cbOffset + i]);
            sum += diff * diff;
            componentCount++;
        }
        // Cr plane
        size_t crOffset = yCount + chromaCount;
        for (size_t i = 0; i < chromaCount; ++i) {
            double diff = static_cast<double>(ref[crOffset + i]) - static_cast<double>(dec[crOffset + i]);
            sum += diff * diff;
            componentCount++;
        }
    } else {
        // Unknown format: treat as single-component
        size_t byteCount = width * height;
        for (size_t i = 0; i < byteCount; ++i) {
            double diff = static_cast<double>(ref[i]) - static_cast<double>(dec[i]);
            sum += diff * diff;
            componentCount++;
        }
    }

    if (componentCount == 0) {
        return 0.0; // Avoid division by zero
    }
    return std::sqrt(sum / componentCount);
}

void vts_videodecoder_helper_validateCapabilities(
    int32_t decoderId,
    const Capabilities& capabilities)
{
    //UT_LOG_INFO("Validating capabilities for decoder ID: %d", decoderId);

    // Basic validation: non-empty codecs and dynamic ranges
    UT_ASSERT_TRUE(!capabilities.supportedCodecs.empty());
    //UT_LOG_INFO("Capabilities has %zu codecs", capabilities.supportedCodecs.size());

    // Validate supportedCodecs
    for (const auto& codecCap : capabilities.supportedCodecs)
    {
        UT_ASSERT_TRUE(codecCap.maxFrameRate > 0);
        UT_ASSERT_TRUE(codecCap.maxFrameWidth > 0);
        UT_ASSERT_TRUE(codecCap.maxFrameHeight > 0);
        UT_LOG_INFO("Codec %d: rate=%d, width=%d, height=%d",
                    static_cast<int32_t>(codecCap.codec),
                    codecCap.maxFrameRate,
                    codecCap.maxFrameWidth,
                    codecCap.maxFrameHeight);
    }

    // Validate supportedDynamicRanges
    UT_ASSERT_TRUE(!capabilities.supportedDynamicRanges.empty());
    //UT_LOG_INFO("Capabilities has %zu dynamic ranges", capabilities.supportedDynamicRanges.size());

    // Optional: Validate against gVideodecoders_hfp (if populated)
    for (const auto& hfpDecoder : gVideodecoders_hfp)
    {
        if (hfpDecoder.id == decoderId)
        {
            //UT_LOG_INFO("Found HFP entry for decoder ID: %d", decoderId);
            UT_ASSERT_EQUAL(capabilities.supportsSecure, hfpDecoder.supportsSecure);
            UT_ASSERT_EQUAL(capabilities.supportedCodecs.size(), hfpDecoder.supportedCodecs.size());
            UT_ASSERT_EQUAL(capabilities.supportedDynamicRanges.size(), hfpDecoder.supportedDynamicRanges.size());

            // Validate codecs
            for (size_t i = 0; i < capabilities.supportedCodecs.size(); ++i)
            {
                const auto& cap = capabilities.supportedCodecs[i];
                const auto& hfpCap = hfpDecoder.supportedCodecs[i];
                UT_ASSERT_EQUAL(static_cast<int32_t>(cap.codec), static_cast<int32_t>(hfpCap.codec));
                UT_ASSERT_EQUAL(cap.maxFrameRate, hfpCap.maxFrameRate);
                UT_ASSERT_EQUAL(cap.maxFrameWidth, hfpCap.maxFrameWidth);
                UT_ASSERT_EQUAL(cap.maxFrameHeight, hfpCap.maxFrameHeight);
            }

            // Validate dynamic ranges
            for (size_t i = 0; i < capabilities.supportedDynamicRanges.size(); ++i)
            {
                UT_ASSERT_EQUAL(static_cast<int32_t>(capabilities.supportedDynamicRanges[i]),
                                static_cast<int32_t>(hfpDecoder.supportedDynamicRanges[i]));
            }
            break;
        }
    }
}

void* vts_kvp_create_instance(char *fileName)
{
    ut_kvp_instance_t* instance;
    ut_kvp_status_t result;

    UT_LOG_INFO("In %s", __FUNCTION__);

    instance = ut_kvp_createInstance();

    if (!instance)
    {
        UT_LOG_ERROR("Failed to create ut kvp instance");
        return nullptr;
    }

    result = ut_kvp_open(instance, fileName);

    if (result != UT_KVP_STATUS_SUCCESS)
    {
        UT_LOG_ERROR("Failed to open test configuration file");
        ut_kvp_destroyInstance(instance);
        return nullptr;
    }

    UT_LOG_INFO("Out %s", __FUNCTION__);

    return (ut_kvp_instance_t*)instance;
}

void vts_kvp_destroy_instance(void *instance)
{
    ut_kvp_destroyInstance((ut_kvp_instance_t*)instance);
}

UT_status_t vts_videodecoder_parse_testconfig(void* pTestInstance, char *module, char *testGroup, char *testName, vector<testConfiguration_t>& testConfiguration)
{
    ut_kvp_status_t status;
    ut_kvp_instance_t* kvpTestInstance = (ut_kvp_instance_t*)pTestInstance;
    std::string streamInputPath;
    std::string streamRefPath;
    int32_t streamCount = 0;
    filesystem::path currentPath = filesystem::current_path();
    filesystem::path fullPath;
    string testList = string(module) + "." + string(testGroup) + "." + string(testName);
    string controlList = testList + "." + "Controls";
    string streamList = testList + "." + "Inputs" + "." + "Streams";
    string streamsDetails = string("VideoStreams");

    // Get input and reference paths
    char tempBuffer[UT_KVP_MAX_ELEMENT_SIZE] = {0};
    status = ut_kvp_getStringField(kvpTestInstance, "IVideoDecoderManager.input_streams_path", tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
    if (status != UT_KVP_STATUS_SUCCESS)
    {
        UT_LOG_ERROR("Failed to get input_streams_path, status: %d", status);
        return UT_STATUS_FAILURE;
    }
    streamInputPath = tempBuffer;
    UT_LOG_INFO("streamInputPath: %s", streamInputPath.c_str());

    status = ut_kvp_getStringField(kvpTestInstance, "IVideoDecoderManager.reference_streams_path", tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
    if (status != UT_KVP_STATUS_SUCCESS)
    {
        UT_LOG_ERROR("Failed to get reference_streams_path, status: %d", status);
        return UT_STATUS_FAILURE;
    }
    streamRefPath = tempBuffer;
    UT_LOG_INFO("streamRefPath: %s", streamRefPath.c_str());

    streamCount = ut_kvp_getListCount(kvpTestInstance, streamList.c_str());
    if (streamCount <= 0)
    {
        UT_LOG_ERROR("No streams found in %s", streamList.c_str());
        return UT_STATUS_FAILURE;
    }
    UT_LOG_INFO("Found %d streams", streamCount);

    for (int32_t i = 0; i < streamCount; i++)
    {
        std::string resultKVP;
        testConfiguration_t testDetails;
        vector<int32_t> errorFrameNum;

        string streamName = streamList + "." + to_string(i) + ".Name";
        status = ut_kvp_getStringField(kvpTestInstance, streamName.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status != UT_KVP_STATUS_SUCCESS)
        {
            UT_LOG_ERROR("Failed to get stream name for index %d, status: %d", i, status);
            continue;
        }
        resultKVP = tempBuffer;
        UT_LOG_INFO("Stream name: %s", resultKVP.c_str());
        testDetails.streamName = resultKVP.c_str();

        string prefix = streamsDetails + "." + resultKVP + ".";
        string input = prefix + "input";
        string reference = prefix + "reference";
        string codec = prefix + "codec";
        string codedWidth = prefix + "codedWidth";
        string codedHeight = prefix + "codedHeight";
        string frameRateNumerator = prefix + "frameRateNumerator";
        string frameRateDenominator = prefix + "frameRateDenominator";
        string pixelFormat = prefix + "pixelFormat";
        string dynamicRange = prefix + "dynamic_range";
        string scanType = prefix + "dynamic_range";
        string colorDepth = prefix + "colorDepth";
        string bit_rate = prefix + "bit_rate";
        string noOfFrames = prefix + "no_of_frames";
        string decodeErrors = prefix + "decodeErrors";
        string framesDropped = prefix + "framesDropped";
        string frameSize = prefix + "frame_size";
        string errorFrames = prefix + "error_frames";
        string framePTS = prefix  + "PTS";
        string rmsDiff = streamList + "." + to_string(i) + ".rms_diff_threshold";
        string decodeTime = streamList + "." + to_string(i) + ".decode_time_threshold";
        string decoderDelay = controlList + ".delay_between_decode_calls";
        string stopPts = prefix + "stop_start_pts";
        string discontinuityPts = prefix + "signal_discontinuity_pts";
        string flushPts = prefix + "flush_pts";
        string flushResetPts = prefix + "flush_reset_pts";
        string eosPts = prefix + "eos_pts";
        string endOfStream = prefix + "endOfStream";
        string discontinuity = prefix + "discontinuity";
        string lowLatency = prefix + "lowLatency";

        // Input stream path
        status = ut_kvp_getStringField(kvpTestInstance, input.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            resultKVP = tempBuffer;
            fullPath = filesystem::path(streamInputPath) / resultKVP;
            UT_LOG_INFO("Full input file path: %s", fullPath.string().c_str());

            if (!filesystem::exists(fullPath))
            {
                UT_LOG_ERROR("Input file does not exist: %s", fullPath.string().c_str());
                continue;
            }

            testDetails.input = fullPath.string();
            UT_LOG_INFO("Input path set: %s", testDetails.input.c_str());
        }
        else
        {
            UT_LOG_ERROR("Failed to get input path for stream %s, status: %d", resultKVP.c_str(), status);
            continue;
        }

        // Codec
        status = ut_kvp_getStringField(kvpTestInstance, codec.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.codec = videoDecoderStringToCodec(string(tempBuffer));
        }

        // Resolution
        testDetails.codedWidth = ut_kvp_getUInt32Field(kvpTestInstance, codedWidth.c_str());
        testDetails.codedHeight = ut_kvp_getUInt32Field(kvpTestInstance, codedHeight.c_str());

        // Frame rate
        testDetails.frameRateNumerator = ut_kvp_getUInt32Field(kvpTestInstance, frameRateNumerator.c_str());
        testDetails.frameRateDenominator = ut_kvp_getUInt32Field(kvpTestInstance, frameRateDenominator.c_str());

        // Pixel format
        status = ut_kvp_getStringField(kvpTestInstance, pixelFormat.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.pixelFormat = videoDecoderStringToPixelFormat(string(tempBuffer));
        }

        // Dynamic range
        status = ut_kvp_getStringField(kvpTestInstance, dynamicRange.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.dynamicRange = videoDecoderStringToDynamicRange(string(tempBuffer));
        }

        // scanType
        status = ut_kvp_getStringField(kvpTestInstance, scanType.c_str(), tempBuffer, UT_KVP_MAX_ELEMENT_SIZE);
        if (status == UT_KVP_STATUS_SUCCESS)
        {
            testDetails.scanType = videoDecoderStringToScanType(string(tempBuffer));
        }

        // Color depth
        testDetails.colorDepth = ut_kvp_getUInt32Field(kvpTestInstance, colorDepth.c_str());

        // Bit rate
        testDetails.bit_rate = ut_kvp_getUInt32Field(kvpTestInstance, bit_rate.c_str());

        // Number of frames
        testDetails.no_of_frames = ut_kvp_getUInt32Field(kvpTestInstance, noOfFrames.c_str());

        // Number of Error frames
        testDetails.decodeErrors = ut_kvp_getUInt32Field(kvpTestInstance, decodeErrors.c_str());

        // Number of Dropped frames
        testDetails.framesDropped = ut_kvp_getUInt32Field(kvpTestInstance, framesDropped.c_str());

        /* Error frames
        if (ut_kvp_fieldPresent(kvpTestInstance, errorFrames.c_str()))
        {
            int32_t count = ut_kvp_getListCount(kvpTestInstance, errorFrames.c_str());
            for (int32_t j = 0; j < count; j++)
            {
                string error = errorFrames + "." + to_string(j);
                int32_t frameNum = ut_kvp_getUInt32Field(kvpTestInstance, error.c_str());
                if (frameNum >= 1 && frameNum <= testDetails.no_of_frames)
                {
                    errorFrameNum.push_back(frameNum);
                }
                else
                {
                    UT_LOG_WARNING("Invalid error frame index: %d, max: %d", frameNum, testDetails.no_of_frames);
                }
            }
        }*/

        // Frame details
        int32_t frameCount = ut_kvp_getListCount(kvpTestInstance, frameSize.c_str());
        UT_LOG_INFO("frameCount %d ",testDetails.framesDropped);


        // this check is not required as no_of_frames and size,pts are different
        /*if (frameCount != testDetails.no_of_frames || frameCount != ut_kvp_getListCount(kvpTestInstance, framePTS.c_str()))
        {
            UT_LOG_ERROR("Frame size/PTS count mismatch: frames=%d, sizes=%d, PTS=%d",
                         testDetails.no_of_frames, frameCount, ut_kvp_getListCount(kvpTestInstance, framePTS.c_str()));
            continue;
        }*/
        for (int32_t j = 0; j < frameCount; j++)
        {
            string eachFrame = frameSize + "." + to_string(j);
            string eachPTS = framePTS + "." + to_string(j);
            int32_t size = ut_kvp_getUInt32Field(kvpTestInstance, eachFrame.c_str());
            int64_t PTS = ut_kvp_getUInt64Field(kvpTestInstance, eachPTS.c_str());
            bool errorFrame = (find(errorFrameNum.begin(), errorFrameNum.end(), j + 1) != errorFrameNum.end());
            testDetails.frameDetails.push_back({size, PTS, errorFrame});
            testDetails.expectedPts.push_back(PTS); // Store PTS for validation
        }

        // RMS difference threshold
        testDetails.rms_diff_threshold = ut_kvp_getDoubleField(kvpTestInstance, rmsDiff.c_str());

        // Decode time threshold
        testDetails.decode_time_threshold = ut_kvp_getUInt64Field(kvpTestInstance, decodeTime.c_str());

        // Decoder delay
        testDetails.decoderDelay = ut_kvp_getUInt32Field(kvpTestInstance, decoderDelay.c_str());

        // Control points
        testDetails.stopPts = ut_kvp_getUInt64Field(kvpTestInstance, stopPts.c_str());
        testDetails.discontinuityPts = ut_kvp_getUInt64Field(kvpTestInstance, discontinuityPts.c_str());
        testDetails.flushPts = ut_kvp_getUInt64Field(kvpTestInstance, flushPts.c_str());
        testDetails.flushResetPts = ut_kvp_getUInt64Field(kvpTestInstance, flushResetPts.c_str());
        testDetails.eosPts = ut_kvp_getUInt64Field(kvpTestInstance, eosPts.c_str());

        testDetails.endOfStream = ut_kvp_getUInt32Field(kvpTestInstance,endOfStream.c_str());
        testDetails.discontinuity = ut_kvp_getUInt32Field(kvpTestInstance,discontinuity.c_str());
        testDetails.lowLatency = ut_kvp_getUInt32Field(kvpTestInstance,lowLatency.c_str());

        testConfiguration.push_back(testDetails);
    }

    return UT_STATUS_OK;
}

/** @} */ // End of Video_Decoder_Halif_Test_Util
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK