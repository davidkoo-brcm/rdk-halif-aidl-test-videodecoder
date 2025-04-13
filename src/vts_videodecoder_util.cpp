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

/** @} */ // End of Video_Decoder_Halif_Test_Util
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK