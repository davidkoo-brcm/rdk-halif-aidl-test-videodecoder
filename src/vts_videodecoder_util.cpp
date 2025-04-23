/**
*  If not stated otherwise in this file or this component's LICENSE
*  file the following copyright and licenses apply:
*
*  Copyright 2024 RDK Management
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



extern sp<IVideoDecoderManager> gVideoDecoderManager;
std::vector<decoder_t> gVideodecoders_hfp;

UT_status_t getVideoDecoderService() {
    gVideoDecoderManager = android::interface_cast<IVideoDecoderManager>(
       defaultServiceManager()->getService(String16(IVideoDecoderManager::serviceName().c_str())));
   if (gVideoDecoderManager == nullptr) {
       printf("Service not available\n");
       return UT_STATUS_FAILURE;
   }
   return UT_STATUS_OK;
}

Codec stringToCodec(const std::string& codecStr) {
    static const std::unordered_map<std::string, Codec> codecMap = {
        {"MPEG", Codec::MPEG2_VIDEO}, {"H264", Codec::H264_AVC}, {"H265", Codec::H265_HEVC},
        {"VP9", Codec::VP9}, {"AV1", Codec::AV1}
    };
    auto it = codecMap.find(codecStr);
    if (it != codecMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Unknown codec: " + codecStr);
    }
}

DynamicRange stringToDynamicRange(const std::string& rangeStr) {
    static const std::unordered_map<std::string, DynamicRange> dynamicRangeMap = {
        {"UNKNOWN", DynamicRange::UNKNOWN}, {"SDR", DynamicRange::SDR},
        {"HLG", DynamicRange::HLG}, {"HDR10", DynamicRange::HDR10},
        {"HDR10PLUS", DynamicRange::HDR10PLUS}, {"DOLBY_VISION", DynamicRange::DOLBY_VISION}
    };
    auto it = dynamicRangeMap.find(rangeStr);
    return (it != dynamicRangeMap.end()) ? it->second : DynamicRange::UNKNOWN;
}

OperationalMode stringToOperationalMode(const std::string& modeStr) {
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

    //UT_LOG_DEBUG("Parsing HFP from: %s", videodecoderpath.c_str());

    number_of_videodecoders = UT_KVP_PROFILE_GET_LIST_COUNT(videodecoderpath.c_str());
    //UT_LOG_DEBUG("Number of video decoders: %d", number_of_videodecoders);
    if (number_of_videodecoders <= 0)
    {
        UT_LOG_ERROR("No videodecoders found in HFP at path: %s", videodecoderpath.c_str());
    }

    supportedopmode_count = UT_KVP_PROFILE_GET_LIST_COUNT(opmodepath.c_str());
    //UT_LOG_DEBUG("Operational modes count: %d", supportedopmode_count);
    temporary.supportedOperationalModes.clear();
    for (int32_t j = 0; j < supportedopmode_count; ++j)
    {
        char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
        std::string modepath = opmodepath + "." + std::to_string(j);
        UT_KVP_PROFILE_GET_STRING(modepath.c_str(), result_kvp);
        std::string modeStr = std::string(result_kvp);
        //UT_LOG_DEBUG("Reading opmode path: %s, value: %s", modepath.c_str(), modeStr.c_str());
        if (!modeStr.empty()) {
            try {
                temporary.supportedOperationalModes.push_back(stringToOperationalMode(modeStr));
            } catch (const std::invalid_argument& e) {
                UT_LOG_WARNING("Invalid opmode at path: %s, value: %s, error: %s", modepath.c_str(), modeStr.c_str(), e.what());
            }
        } else {
            UT_LOG_WARNING("Empty opmode at path: %s", modepath.c_str());
        }
    }
    //UT_LOG_DEBUG("Parsed %zu operational modes", temporary.supportedOperationalModes.size());

    const std::vector<std::string> known_codecs = {"MPEG", "H264", "H265", "VP9", "AV1"};

    gVideodecoders_hfp.clear();
    for (int32_t i = 0; i < number_of_videodecoders; ++i)
    {
        std::string decoderbasepath = videodecoderpath + "." + std::to_string(i);
        std::string supportedcodecpath = decoderbasepath + ".supportedCodecs";
        std::string supporteddrpath = decoderbasepath + ".supportedDynamicRanges";
        std::string supportsSecurepath = decoderbasepath + ".supportsSecure";

        temporary.id = i;
        temporary.supportsSecure = false;
        temporary.supportedCodecs.clear();
        temporary.supportedDynamicRanges.clear();

        temporary.supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
        //UT_LOG_DEBUG("Decoder %d: id=%d, supportsSecure=%d", i, temporary.id, temporary.supportsSecure);

        supportedcodec_count = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcodecpath.c_str());
        //UT_LOG_DEBUG("Decoder %d: codec count=%d", i, supportedcodec_count);
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
                    //UT_LOG_DEBUG("Found codec at path: %s, codec: %s", codecpath.c_str(), codec_name.c_str());
                    try {
                        cap.codec = stringToCodec(codec_name);
                    } catch (const std::invalid_argument& e) {
                        UT_LOG_WARNING("Invalid codec: %s, error: %s, skipping", codec_name.c_str(), e.what());
                        continue;
                    }

                    cap.maxFrameRate = test_rate;
                    cap.maxFrameWidth = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameWidth").c_str());
                    cap.maxFrameHeight = UT_KVP_PROFILE_GET_UINT32((attrpath + ".maxFrameHeight").c_str());
                    //UT_LOG_DEBUG("Codec %s: rate=%d, width=%d, height=%d",
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
        //UT_LOG_DEBUG("Decoder %d: dynamic range count=%d", i, supporteddr_count);
        for (int32_t j = 0; j < supporteddr_count; ++j)
        {
            char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
            std::string drpath = supporteddrpath + "." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(drpath.c_str(), result_kvp);
            std::string drStr = std::string(result_kvp);
            //UT_LOG_DEBUG("Reading dynamic range path: %s, value: %s", drpath.c_str(), drStr.c_str());
            if (!drStr.empty()) {
                try {
                    temporary.supportedDynamicRanges.push_back(stringToDynamicRange(drStr));
                } catch (const std::invalid_argument& e) {
                    UT_LOG_WARNING("Invalid dynamic range at path: %s, value: %s, error: %s",
                                   drpath.c_str(), drStr.c_str(), e.what());
                }
            } else {
                UT_LOG_WARNING("Empty dynamic range at path: %s", drpath.c_str());
            }
        }

        //UT_LOG_DEBUG("Decoder %d: id=%d, %zu codecs, %zu dynamic ranges, %zu opmodes",
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

    //UT_LOG_DEBUG("Successfully parsed %zu decoders", gVideodecoders_hfp.size());
    return UT_STATUS_OK;
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

/** @} */ // End of Video_Decoder_Halif_Test_Util
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK