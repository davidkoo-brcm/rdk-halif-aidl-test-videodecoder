
/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 #include "vts_videodecoder.h"

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
/**
 * @brief Test the retrieval of video decoder IDs from the VideoDecoderManager service.
 *
 * This test verifies that the VideoDecoderManager service can successfully return a list of video decoder IDs. It checks the status of the operation, ensures that the list is not empty, and validates that the number of IDs matches the expected count. Additionally, it confirms that each expected ID is present in the retrieved list.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 001@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize decoderIds vector to store video decoder IDs. | decoderIds = empty vector | Should be successful | |
 * | 02 | Call getVideoDecoderIds to retrieve video decoder IDs. | OUT: decoderIds | status.isOk() = true, decoderIds not empty | Should Pass |
 * | 03 | Log the result of getVideoDecoderIds call. | decoderIds.size(), status | Log should show size and status | Should be successful |
 * | 04 | Assert that the status is OK. | status.isOk() | Assertion should pass | Should Pass |
 * | 05 | Assert that decoderIds is not empty. | decoderIds | Assertion should pass | Should Pass |
 * | 06 | Retrieve expected count of video decoder IDs. | expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT("IVideoDecoderManager.IVideoDecoder") | Should be successful | |
 * | 07 | Assert that the size of decoderIds matches expected count. | decoderIds.size(), expectedCount | Assertion should pass | Should Pass |
 * | 08 | Iterate over expected IDs and verify each is present in decoderIds. | expectedId, decoderIds | Assertion should pass for each ID | Should Pass |
 */
UT_ADD_TEST(VideoDecoderManagerBaseTest, test_l1_VideoDecoderManager_positive_getVideoDecoderIds) {
    std::vector<::com::rdk::hal::videodecoder::IVideoDecoder::Id> decoderIds;
    size_t expectedCount = 0;

    UT_ASSERT_TRUE(service != nullptr) << "Service pointer is null.";
    UT_LOG_INFO("Calling getVideoDecoderIds(OUT:decoderIds[])");
    android::binder::Status status = service->getVideoDecoderIds(&decoderIds);

    UT_LOG_INFO("Result getVideoDecoderIds(OUT:decoderIds.size:[%zu]), status:[isOK:%s, exception:%s]",
                decoderIds.size(),
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_TRUE(status.isOk()) << "Failed to retrieve video decoder IDs.";
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder IDs found.";

    std::string videodecoder_count_path = "IVideoDecoderManager.IVideoDecoder"; // Verify this path
    expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT(videodecoder_count_path.c_str());
    UT_LOG_INFO("Expected count from profile: %zu", expectedCount);
    UT_ASSERT_TRUE(expectedCount > 0) << "Profile returned invalid count.";
    UT_ASSERT_EQUAL(decoderIds.size(), expectedCount) << "Decoder ID count mismatch.";

    for (size_t i = 0; i < expectedCount; ++i) {
        std::string videodecoderidpath = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".id";
        int32_t expectedId = UT_KVP_PROFILE_GET_UINT32(videodecoderidpath.c_str());
        UT_LOG_INFO("Checking ID at %s: %d", videodecoderidpath.c_str(), expectedId);
        auto it = std::find_if(decoderIds.begin(), decoderIds.end(), [expectedId](const auto& id) {
            return id.value == expectedId;  // Adjust if Id.value isn’t correct
        });
        UT_ASSERT_TRUE(it != decoderIds.end()) << "Expected VideoDecoder ID not found: " << expectedId;
    }
}



/**
 * @brief Test to verify the retrieval of supported operational modes from the VideoDecoderManager.
 *
 * This test checks the functionality of the getSupportedOperationalModes API to ensure it correctly retrieves
 * the list of supported operational modes. It verifies that the API call is successful, the list is not empty,
 * and the modes match the expected configuration.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 002@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize supportedModes vector to store operational modes. | supportedModes = empty vector | Should be successful | Should be successful |
 * | 02 | Call getSupportedOperationalModes to retrieve supported modes. | supportedModes = empty vector | status.isOk() = true, supportedModes not empty | Should Pass |
 * | 03 | Verify the size of supportedModes matches expected count from configuration. | supportedModes.size() = actual size, expectedCount = value from config | supportedModes.size() == expectedCount | Should Pass |
 * | 04 | Iterate through each expected mode and verify it exists in supportedModes. | expectedMode = value from config, supportedModes = list of modes | Each expectedMode found in supportedModes | Should Pass |
 */
UT_ADD_TEST(VideoDecoderManagerBaseTest, test_l1_VideoDecoderManager_positive_getSupportedOperationalModes) {
    android::binder::Status status;
    std::vector<::com::rdk::hal::videodecoder::OperationalMode> supportedModes;
    UT_LOG_INFO("Calling getSupportedOperationalModes(OUT:supportedModes[])");
    status = service->getSupportedOperationalModes(&supportedModes);
    UT_LOG_INFO("Result getSupportedOperationalModes(OUT:supportedModes[%zu]), status:[isOK:%s, exception:%s]",
                supportedModes.size(),
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_TRUE(status.isOk()) << "Failed to get supported operational modes.";
    UT_ASSERT_FALSE(supportedModes.empty()) << "Supported operational modes list is empty.";

    std::string operationalmode_count_path = "IVideoDecoderManager.supportedOperationalModes";
    size_t expectedCount = UT_KVP_PROFILE_GET_LIST_COUNT(operationalmode_count_path.c_str());
    UT_ASSERT_EQUAL(supportedModes.size(), expectedCount) << "Mismatch in number of supported operational modes.";

    for (size_t i = 0; i < expectedCount; ++i) {
        char pszReturnedString[UT_KVP_MAX_ELEMENT_SIZE];
        std::string operationalmode_path = "IVideoDecoderManager.supportedOperationalModes." + std::to_string(i);
        UT_KVP_PROFILE_GET_STRING(operationalmode_path.c_str(), pszReturnedString);
        OperationalMode expectedMode;
        if (strcmp(pszReturnedString, "TUNNELLED") == 0) {
            expectedMode = OperationalMode::TUNNELLED;
        } else if (strcmp(pszReturnedString, "NON_TUNNELLED") == 0) {
            expectedMode = OperationalMode::NON_TUNNELLED;
        } else if (strcmp(pszReturnedString, "GRAPHICS_TEXTURE") == 0) {
            expectedMode = OperationalMode::GRAPHICS_TEXTURE;
        } else {
            UT_FAIL("Unsupported operational mode in YAML configuration.");
        }
        UT_ASSERT_TRUE(std::find(supportedModes.begin(), supportedModes.end(), expectedMode) != supportedModes.end())
            << "Operational mode " << pszReturnedString << " not found in supported modes.";
    }
}



/**
 * @brief Test the retrieval of video decoder instances using valid decoder IDs.
 *
 * This test verifies the functionality of retrieving video decoder instances from the service using valid decoder IDs obtained from a YAML configuration. It ensures that the service correctly returns non-null video decoder objects and that the operation status is successful.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 003@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve Video Decoder IDs from YAML configuration | decoderIds = empty | decoderIds should not be empty | Should be successful |
 * | 02 | Prepare the videoDecoderId using the macro | videoDecoderId.value = UT_KVP_PROFILE_GET_UINT32("IVideoDecoderManager.IVideoDecoder.<index>.0") | videoDecoderId should be valid | Should be successful |
 * | 03 | Call getVideoDecoder with the valid videoDecoderId | IN: videoDecoderId, OUT: videoDecoder | videoDecoder should not be null | Should Pass |
 * | 04 | Verify that the return value is ::android::binder::Status::ok() | status = service->getVideoDecoder(videoDecoderId, &videoDecoder) | status.isOk() should be true | Should Pass |
 * | 05 | Log the result | videoDecoderId.value, videoDecoder.get(), status.isOk(), status.exceptionCode() | Log should reflect correct values | Should be successful |
 */
UT_ADD_TEST(VideoDecoderManagerBaseTest, test_l1_VideoDecoderManager_positive_getVideoDecoder) {
    // Retrieve Video Decoder IDs from YAML configuration
    std::vector<IVideoDecoder::Id> decoderIds;
    UT_ASSERT_TRUE(service->getVideoDecoderIds(&decoderIds).isOk());
    UT_ASSERT_FALSE(decoderIds.empty()) << "No VideoDecoder IDs found.";
    int i=0;

    for (const auto& id : decoderIds) {
        // Prepare the videoDecoderId using the macro
        IVideoDecoder::Id videoDecoderId;
        std::string decoder_id_path = "IVideoDecoderManager.IVideoDecoder."+std::to_string(i)+".id";
        videoDecoderId.value = UT_KVP_PROFILE_GET_UINT32(decoder_id_path.c_str());
        i++;

        // Call getVideoDecoder with the valid videoDecoderId
        sp<IVideoDecoder> videoDecoder;
        UT_LOG_INFO("Calling getVideoDecoder(IN:videoDecoderId:[%d], OUT:videoDecoder[])", videoDecoderId.value);
        android::binder::Status status = service->getVideoDecoder(videoDecoderId, &videoDecoder);

        // Verify that _aidl_return is not null
        UT_ASSERT_TRUE(videoDecoder != nullptr) << "IVideoDecoder object is null for ID: " << videoDecoderId.value;

        // Verify that the return value is ::android::binder::Status::ok()
        UT_ASSERT_TRUE(status.isOk()) << "getVideoDecoder failed for ID: " << videoDecoderId.value;

        // Log the result
        UT_LOG_INFO("Result getVideoDecoder(IN:videoDecoderId:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]",
                    videoDecoderId.value, videoDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
    }
}



/**
 * @brief Test the behavior of getVideoDecoder with invalid inputs
 *
 * This test evaluates the getVideoDecoder function's response to invalid inputs, ensuring that it handles errors gracefully and returns appropriate status codes.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 004@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize an invalid Video Decoder ID using UNDEFINED | invalidDecoderId.value = IVideoDecoder::Id::UNDEFINED | videoDecoder should be nullptr, status.exceptionCode() should be EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 02 | Call getVideoDecoder with invalid Video Decoder ID | input: invalidDecoderId, output: videoDecoder | videoDecoder should be nullptr, status.exceptionCode() should be EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 03 | Initialize a valid Video Decoder ID from configuration | validDecoderId.value = UT_KVP_PROFILE_GET_UINT32("IVideoDecoderManager.IVideoDecoder.<index>.0") | Should be successful | Should be successful |
 * | 04 | Call getVideoDecoder with a valid Video Decoder ID and null videoDecoder instance | input: validDecoderId, output: nullptr | videoDecoder should be nullptr, status.exceptionCode() should be EX_NULL_POINTER | Should Fail |
 */
UT_ADD_TEST(VideoDecoderManagerBaseTest, test_l1_VideoDecoderManager_negative_getVideoDecoder) {
    // Test Case 1: Invalid Video Decoder ID
    IVideoDecoder::Id invalidDecoderId;
    invalidDecoderId.value = IVideoDecoder::Id::UNDEFINED; // Using UNDEFINED as invalid ID
    sp<IVideoDecoder> videoDecoder;

    UT_LOG_INFO("Calling getVideoDecoder(IN:videoDecoderId:[%d], OUT:videoDecoder[])", invalidDecoderId.value);
    android::binder::Status status = service->getVideoDecoder(invalidDecoderId, &videoDecoder);
    UT_LOG_INFO("Result getVideoDecoder(IN:videoDecoderId:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]",
                invalidDecoderId.value, videoDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_EQUAL(videoDecoder, nullptr);

    // Test Case 2: Null instance of IVideoDecoder
    IVideoDecoder::Id validDecoderId;
    std::string decoder_id_path = "IVideoDecoderManager.IVideoDecoder."+std::to_string(0)+".id";
    validDecoderId.value = UT_KVP_PROFILE_GET_UINT32(decoder_id_path.c_str());

    UT_LOG_INFO("Calling getVideoDecoder(IN:videoDecoderId:[%d], videoDecoder = nullptr)", validDecoderId.value);
    status = service->getVideoDecoder(validDecoderId, nullptr);
    UT_LOG_INFO("Result getVideoDecoder(IN:videoDecoderId:[%d], OUT:videoDecoder[0x%X]), status:[isOK:%s, exception:%s]",
                validDecoderId.value, videoDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

    UT_ASSERT_EQUAL(videoDecoder, nullptr);
}



/**
 * @brief Test the positive scenario for retrieving video decoder capabilities.
 *
 * This test verifies the functionality of the `getCapabilities` method for video decoders. It checks if the method correctly retrieves the capabilities of each decoder, including supported codecs, dynamic ranges, and secure support.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 005@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and retrieve the list of video decoders. | decoders = list of video decoders | Decoders list should be initialized | Should be successful |
 * | 02 | Iterate over each decoder and call the `getCapabilities` method. | decoder[i], capabilities = OUT | Status should be OK | Should Pass |
 * | 03 | Verify the count of supported codecs against the expected count from the profile. | codecCount = expected count | Supported codecs count should match | Should Pass |
 * | 04 | For each codec, verify the codec type and its attributes like maxFrameRate, maxFrameWidth, and maxFrameHeight. | codecCapability, maxFrameRate, maxFrameWidth, maxFrameHeight | Codec type should be valid, attributes should match | Should Pass |
 * | 05 | Verify the count of supported dynamic ranges against the expected count from the profile. | dynamicRangeCount = expected count | Supported dynamic ranges count should match | Should Pass |
 * | 06 | For each dynamic range, verify its presence in the capabilities. | dynamicRangeValue | Dynamic range should be found | Should Pass |
 * | 07 | Verify the supportsSecure flag against the expected value from the profile. | supportsSecure = expected value | SupportsSecure flag should match | Should Pass |
 */

UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_getCapabilities) {
    for (size_t i = 0; i < decoders.size(); ++i) {
        ::com::rdk::hal::videodecoder::Capabilities capabilities;

        UT_LOG_INFO("Calling getCapabilities(OUT:capabilities)");
        android::binder::Status status = decoders[i]->getCapabilities(&capabilities);

        UT_ASSERT_TRUE(status.isOk()) << "getCapabilities call failed";
        UT_LOG_INFO("Result getCapabilities(OUT:capabilities), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Verify supportedCodecs
        std::string supportedcodec_count_path = "IVideoDecoderManager.IVideoDecoder."+std::to_string(i)+".supportedCodecs";
        size_t codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcodec_count_path.c_str());
        UT_ASSERT_EQUAL(capabilities.supportedCodecs.size(), codecCount) << "Mismatch in supportedCodecs count";

        for (size_t j = 0; j < codecCount; ++j) {
            std::string codec_path = supportedcodec_count_path+"."+std::to_string(j);
            std::string maxFrameRate_path = supportedcodec_count_path+"."+std::to_string(j) + ".maxFrameRate";
            std::string maxFrameWidth_path = supportedcodec_count_path+"."+std::to_string(j) + ".maxFrameWidth";
            std::string maxFrameHeight_path = supportedcodec_count_path+"."+std::to_string(j) + ".maxFrameHeight";


            char pszReturnedString[UT_KVP_MAX_ELEMENT_SIZE];
            UT_KVP_PROFILE_GET_STRING(codec_path.c_str(), pszReturnedString);

            int32_t maxFrameRate = UT_KVP_PROFILE_GET_UINT32(maxFrameRate_path.c_str());
            int32_t maxFrameWidth = UT_KVP_PROFILE_GET_UINT32(maxFrameWidth_path.c_str());
            int32_t maxFrameHeight = UT_KVP_PROFILE_GET_UINT32(maxFrameHeight_path.c_str());

            const auto& codecCapability = capabilities.supportedCodecs[j];
            //UT_ASSERT_EQUAL(codecCapability.codec, stringToCodec(pszReturnedString)) << "Invalid codec type";
            UT_ASSERT_EQUAL(codecCapability.maxFrameRate, maxFrameRate) << "Mismatch in maxFrameRate";
            UT_ASSERT_EQUAL(codecCapability.maxFrameWidth, maxFrameWidth) << "Mismatch in maxFrameWidth";
            UT_ASSERT_EQUAL(codecCapability.maxFrameHeight, maxFrameHeight) << "Mismatch in maxFrameHeight";
        }

        // Verify supportedDynamicRanges
        std::string dynamicrange_count_path = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedDynamicRanges";
        size_t dynamicRangeCount = UT_KVP_PROFILE_GET_LIST_COUNT(dynamicrange_count_path.c_str());
        UT_ASSERT_EQUAL(capabilities.supportedDynamicRanges.size(), dynamicRangeCount) << "Mismatch in supportedDynamicRanges count";

        for (size_t j = 0; j < dynamicRangeCount; ++j) {
            char dynamicrangestring[UT_KVP_MAX_ELEMENT_SIZE];
            std::string dynamicrange_path = dynamicrange_count_path+"." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(dynamicrange_path.c_str(),dynamicrangestring);

            UT_ASSERT_NOT_EQUAL(std::find(capabilities.supportedDynamicRanges.begin(), capabilities.supportedDynamicRanges.end(), stringToDynamicRange(dynamicrangestring)), capabilities.supportedDynamicRanges.end()) << "DynamicRange value not found";
        }

        // Verify supportsSecure
        std::string supportsecure_path = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
        bool supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsecure_path.c_str());
        UT_ASSERT_EQUAL(capabilities.supportsSecure, supportsSecure) << "Mismatch in supportsSecure flag";
    }
}



/**
 * @brief Test the positive scenario of retrieving properties from VideoDecoder
 *
 * This test verifies the ability of the VideoDecoder to correctly retrieve various properties such as RESOURCE_ID, OPERATIONAL_MODE, SECURE_VIDEO, and METRIC_FRAMES_DECODED. It ensures that the decoder returns the expected property values without any errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 006@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and iterate over each decoder | decoders = list of decoders | Should be successful | |
 * | 02 | Call getProperty with RESOURCE_ID and check the result | property = RESOURCE_ID, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 * | 03 | Call getProperty with OPERATIONAL_MODE and check the result | property = OPERATIONAL_MODE, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 * | 04 | Call getProperty with SECURE_VIDEO and check the result | property = SECURE_VIDEO, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 * | 05 | Call getProperty with METRIC_FRAMES_DECODED and check the result | property = METRIC_FRAMES_DECODED, aidl_return = optional | status.isOk() = true, aidl_return.has_value() = true, aidl_return->value->getTag() = intValue | Should Pass |
 */

UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_getProperty) {
    android::binder::Status status;
    for (const auto& decoder : decoders) {
        ::std::optional<::com::rdk::hal::PropertyValue> aidl_return;

        // Test for RESOURCE_ID
        UT_LOG_INFO("Calling getProperty(IN:property:[RESOURCE_ID], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::videodecoder::Property::RESOURCE_ID, &aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[RESOURCE_ID], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return.has_value());
        UT_ASSERT_EQUAL(aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::intValue);

        // Test for OPERATIONAL_MODE
        UT_LOG_INFO("Calling getProperty(IN:property:[OPERATIONAL_MODE], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE, &aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[OPERATIONAL_MODE], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return.has_value());
        UT_ASSERT_EQUAL(aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::intValue);

        // Test for SECURE_VIDEO
        UT_LOG_INFO("Calling getProperty(IN:property:[SECURE_VIDEO], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::videodecoder::Property::SECURE_VIDEO, &aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[SECURE_VIDEO], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return.has_value());
        UT_ASSERT_EQUAL(aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::intValue);

        // Test for METRIC_FRAMES_DECODED
        UT_LOG_INFO("Calling getProperty(IN:property:[METRIC_FRAMES_DECODED], OUT:_aidl_return[])");
        status = decoder->getProperty(::com::rdk::hal::videodecoder::Property::METRIC_FRAMES_DECODED, &aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[METRIC_FRAMES_DECODED], OUT:_aidl_return[0x%X]), status:[isOK:%s, exception:%s]",
                    aidl_return.has_value(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return.has_value());
        UT_ASSERT_EQUAL(aidl_return->value->getTag(), ::com::rdk::hal::PropertyValue::Value::intValue);
    }
}



/**
 * @brief Test the negative scenario for the VideoDecoder getProperty function
 *
 * This test checks the behavior of the VideoDecoder's getProperty function when an invalid property is requested. It ensures that the function handles unknown properties gracefully and returns the expected error status.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 007@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and prepare the list of decoders | decoders = list of VideoDecoder instances | Should be successful | |
 * | 02 | Iterate over each decoder and call the getProperty function with an invalid property | property = UNKNOWN_PROPERTY, aidl_return = null | status.isOk() = false, aidl_return.has_value() = false | Should Fail |
 * | 03 | Log the result of the getProperty function call | aidl_return = null, status = error status | Should be successful | |
 * | 04 | Assert that the status is not OK and the aidl_return is empty | status.isOk() = false, aidl_return.has_value() = false | Should be successful | |
 */

UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_negative_getProperty) {
    android::binder::Status status;
    for (const auto& decoder : decoders) {
        ::std::optional<::com::rdk::hal::PropertyValue> aidl_return;
        UT_LOG_INFO("Calling getProperty(IN:property:[UNKNOWN_PROPERTY], OUT:_aidl_return[])");
        status = decoder->getProperty(static_cast<::com::rdk::hal::videodecoder::Property>(-1), &aidl_return);
        UT_LOG_INFO("Result getProperty(IN:property:[UNKNOWN_PROPERTY], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                    aidl_return.has_value() ? "not null" : "null",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_FALSE(aidl_return.has_value());
    }
}



/**
 * @brief Test the positive scenario of retrieving multiple properties from the VideoDecoder.
 *
 * This test verifies the functionality of the getPropertyMulti method in the VideoDecoderBaseTest class. 
 * It checks whether multiple properties can be successfully retrieved and populated in the PropertyKVPair list.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 008@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the propertyKVList with valid properties | propertyKVList initialized with RESOURCE_ID, INPUT_QUEUE_DEPTH, OUTPUT_FRAME_POOL_SIZE, OPERATIONAL_MODE, SECURE_VIDEO, METRIC_FRAMES_DECODED, METRIC_DECODE_ERRORS, METRIC_FRAMES_DROPPED | Properties should be initialized successfully | Should be successful |
 * | 02 | Call getPropertyMulti with the initialized propertyKVList | IN: propertyKVList, OUT: _aidl_return | Status should be OK, _aidl_return should be true | Should Pass |
 * | 03 | Verify each PropertyKVPair in the list has its propertyValue correctly populated | propertyKVList | Each propertyValue should have a value | Should be successful |
 */

 UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_getPropertyMulti) {
    for (const auto& decoder : decoders) {
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null.";

        std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVList;
        // Populate propertyKVList with valid properties
        PropertyKVPair kvPair1;
        kvPair1.property = Property::RESOURCE_ID;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair1);

        PropertyKVPair kvPair2;
        kvPair1.property = Property::INPUT_QUEUE_DEPTH;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair2);


        PropertyKVPair kvPair3;
        kvPair1.property = Property::OUTPUT_FRAME_POOL_SIZE;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair3);

        PropertyKVPair kvPair4;
        kvPair1.property = Property::OPERATIONAL_MODE;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair4);

        PropertyKVPair kvPair5;
        kvPair1.property = Property::SECURE_VIDEO;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair5);

        PropertyKVPair kvPair6;
        kvPair1.property = Property::METRIC_FRAMES_DECODED;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair6);

        PropertyKVPair kvPair7;
        kvPair1.property = Property::METRIC_DECODE_ERRORS;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair7);

        PropertyKVPair kvPair8;
        kvPair1.property = Property::METRIC_FRAMES_DROPPED;
        kvPair1.propertyValue.value= PropertyValue::Value();
        propertyKVList.push_back(kvPair8);

        bool _aidl_return = false;

        UT_LOG_INFO("Calling getPropertyMulti(IN:propertyKVList.size:[%zu], OUT:_aidl_return)", propertyKVList.size());
        android::binder::Status status = decoder->getPropertyMulti(&propertyKVList, &_aidl_return);

        UT_LOG_INFO("Result getPropertyMulti(OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, _aidl_return),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        UT_ASSERT_TRUE(status.isOk()) << "Binder call failed: " << status.exceptionMessage().c_str();
        UT_ASSERT_TRUE(_aidl_return) << "getPropertyMulti returned false.";

        // Verify that each PropertyKVPair has its propertyValue correctly populated
        for (const auto& kvPair : propertyKVList) {
            UT_ASSERT_TRUE(kvPair.propertyValue.value.has_value()) << "Property " << static_cast<int>(kvPair.property) << " has no value.";
            // Additional type checking based on Property enum
            switch (kvPair.property) {
                case ::com::rdk::hal::videodecoder::Property::RESOURCE_ID:
                case ::com::rdk::hal::videodecoder::Property::INPUT_QUEUE_DEPTH:
                case ::com::rdk::hal::videodecoder::Property::OUTPUT_FRAME_POOL_SIZE:
                case ::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE:
                case ::com::rdk::hal::videodecoder::Property::METRIC_FRAMES_DECODED:
                case ::com::rdk::hal::videodecoder::Property::METRIC_DECODE_ERRORS:
                case ::com::rdk::hal::videodecoder::Property::METRIC_FRAMES_DROPPED:
                    // Expect intValue (Integer type)
                    // Note: Actual union access depends on implementation (raw union or std::variant)
                    break;
                case ::com::rdk::hal::videodecoder::Property::SECURE_VIDEO:
                    // Expect booleanValue (Integer 0/1, but logically boolean)
                    break;
                default:
                    UT_FAIL("Unexpected property in list: ") << static_cast<int>(kvPair.property);
            }
        }
    }
}



/**
 * @brief Test the behavior of the VideoDecoder when invalid or empty property keys are provided.
 *
 * This test checks the VideoDecoder's response to invalid property keys and empty property lists. It ensures that the decoder handles these scenarios gracefully without crashing and returns expected results.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 009@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize invalid property key pair and add to list | invalidPair.property = -1 | aidlReturnInvalid = false, kvPair.propertyValue.value.has_value() = false | Should Fail |
 * | 02 | Call getPropertyMulti with invalid property list | propertyKVListInvalid, aidlReturnInvalid | statusInvalid.isOk() = false, statusInvalid.exceptionCode() = expected exception | Should Fail |
 * | 03 | Initialize empty property list | propertyKVListEmpty | aidlReturnEmpty = false | Should Fail |
 * | 04 | Call getPropertyMulti with empty property list | propertyKVListEmpty, aidlReturnEmpty | statusEmpty.isOk() = false, statusEmpty.exceptionCode() = expected exception | Should Fail |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_negative_getPropertyMulti) {
    android::binder::Status statusInvalid;
    android::binder::Status statusEmpty;
    // Negative Test - Invalid Property Key
    std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVListInvalid;
    ::com::rdk::hal::videodecoder::PropertyKVPair invalidPair;
    invalidPair.property = static_cast<::com::rdk::hal::videodecoder::Property>(-1); // Invalid property key
    propertyKVListInvalid.push_back(invalidPair);

    bool aidlReturnInvalid = true;
    UT_LOG_INFO("Calling getPropertyMulti(IN:propertyKVListInvalid, OUT:aidlReturnInvalid)");
    statusInvalid = decoders[0]->getPropertyMulti(&propertyKVListInvalid, &aidlReturnInvalid);
    UT_LOG_INFO("Result getPropertyMulti(IN:propertyKVListInvalid, OUT:aidlReturnInvalid[%s]), status:[isOK:%s, exception:%s]",
                UT_Control_GetMapString(bool_mapTable, aidlReturnInvalid),
                UT_Control_GetMapString(bool_mapTable, statusInvalid.isOk()),
                UT_Control_GetMapString(statusException_mapTable, statusInvalid.exceptionCode()));
    UT_ASSERT_FALSE(aidlReturnInvalid);
    for (const auto& kvPair : propertyKVListInvalid) {
        UT_ASSERT_FALSE(kvPair.propertyValue.value.has_value());
    }

    // Negative Test - Empty Property List
    std::vector<::com::rdk::hal::videodecoder::PropertyKVPair> propertyKVListEmpty;
    bool aidlReturnEmpty = true;
    UT_LOG_INFO("Calling getPropertyMulti(IN:propertyKVListEmpty, OUT:aidlReturnEmpty)");
    statusEmpty = decoders[0]->getPropertyMulti(&propertyKVListEmpty, &aidlReturnEmpty);
    UT_LOG_INFO("Result getPropertyMulti(IN:propertyKVListEmpty, OUT:aidlReturnEmpty[%s]), status:[isOK:%s, exception:%s]",
                UT_Control_GetMapString(bool_mapTable, aidlReturnEmpty),
                UT_Control_GetMapString(bool_mapTable, statusEmpty.isOk()),
                UT_Control_GetMapString(statusException_mapTable, statusEmpty.exceptionCode()));
    UT_ASSERT_FALSE(aidlReturnEmpty);
}



/**
 * @brief Test the positive scenario of getting the state of a video decoder.
 *
 * This test verifies that the `getState` function of the video decoder returns a valid state and that the operation completes successfully. It ensures that the decoder is in one of the expected states.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 010@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and prepare the list of decoders. | decoders = list of video decoders | Decoders list should be initialized | Should be successful |
 * | 02 | Iterate over each decoder and call the `getState` function to retrieve the current state. | decoder = current decoder, state = output state | Status should be OK, state should be valid | Should Pass |
 * | 03 | Verify that the returned state is one of the expected states: UNKNOWN, CLOSED, OPENING, READY, STARTING, STARTED, FLUSHING, STOPPING, CLOSING. | state = output state | State should be one of the expected values | Should Pass |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_getState) {
    for (const auto& decoder : decoders) {
        ::com::rdk::hal::State state;
        UT_LOG_INFO("Calling getState(OUT:_aidl_return:[state])");
        android::binder::Status status = decoder->getState(&state);
        UT_LOG_INFO("Result getState(OUT:_aidl_return:[state:%d]), status:[isOK:%s, exception:%s]",
                    static_cast<int32_t>(state),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(state == ::com::rdk::hal::State::UNKNOWN ||
                    state == ::com::rdk::hal::State::CLOSED ||
                    state == ::com::rdk::hal::State::OPENING ||
                    state == ::com::rdk::hal::State::READY ||
                    state == ::com::rdk::hal::State::STARTING ||
                    state == ::com::rdk::hal::State::STARTED ||
                    state == ::com::rdk::hal::State::FLUSHING ||
                    state == ::com::rdk::hal::State::STOPPING ||
                    state == ::com::rdk::hal::State::CLOSING);
    }
}



/**
 * @brief Test the positive scenario of opening a video decoder.
 *
 * This test verifies the ability to open a video decoder with various codecs and secure modes. It ensures that the decoder can be opened successfully and that a valid controller is returned.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 011@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize listener for video decoder controller. | listener = new VideoDecoderControllerListener() | Listener should be initialized successfully. | Should be successful |
 * | 02 | Retrieve secure mode from YAML configuration. | securePathKey = "IVideoDecoderManager.IVideoDecoder.i.supportsSecure" | Secure mode should be retrieved successfully. | Should be successful |
 * | 03 | Retrieve supported codecs count from YAML configuration. | codecCountKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs" | Codec count should be retrieved successfully. | Should be successful |
 * | 04 | Loop through each supported codec and retrieve codec name. | codecPathKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs.j" | Codec name should be retrieved successfully. | Should be successful |
 * | 05 | Map codec name to Codec enum. | codecName = "MPEG"/"H264"/"H265"/"VP9"/"AV1" | Codec should be mapped successfully. | Should be successful |
 * | 06 | Call open function with codec, secure mode, and listener. | codec = Codec::MPEG/H264/H265/VP9/AV1, secure = true/false, listener = VideoDecoderControllerListener | Decoder should open successfully, controller should not be null. | Should Pass |
 * | 07 | Assert that the status is OK and controller is not null. | status.isOk() = true, controller != nullptr | Assertions should pass. | Should Pass |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_open) {
    for (size_t i = 0; i < decoders.size(); ++i) {
        sp<IVideoDecoderController> controller;
        sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();

        // Retrieve secure mode from YAML configuration
        std::string securePathKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
        bool secure = UT_KVP_PROFILE_GET_BOOL(securePathKey.c_str());

        // Retrieve supported codecs from YAML configuration
        std::string codecCountKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs";
        size_t codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(codecCountKey.c_str());

        for (size_t j = 0; j < codecCount; ++j) {
            char codecName[UT_KVP_MAX_ELEMENT_SIZE];
            std::string codecPathKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(codecPathKey.c_str(), codecName);

            Codec codec = Codec::MPEG; // Default value, replace with actual mapping from string to enum
            if (strcmp(codecName, "MPEG") == 0) codec = Codec::MPEG;
            else if (strcmp(codecName, "H264") == 0) codec = Codec::H264;
            else if (strcmp(codecName, "H265") == 0) codec = Codec::H265;
            else if (strcmp(codecName, "VP9") == 0) codec = Codec::VP9;
            else if (strcmp(codecName, "AV1") == 0) codec = Codec::AV1;

            UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[])", static_cast<int32_t>(codec), secure, listener.get());
            android::binder::Status status = decoders[i]->open(codec, secure, listener, &controller);
            UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int32_t>(codec), secure, listener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(controller != nullptr);

            // Calling close API
            bool isClosed = false;
            UT_LOG_INFO("Calling close(IN:controller[0x%X], OUT:_aidl_return[])", controller.get());
            status = decoders[i]->close(controller, &isClosed);
            // Result close
            UT_LOG_INFO("Result close(IN:videoDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                        controller.get(), UT_Control_GetMapString(bool_mapTable, isClosed),
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(isClosed);
        }
    }
}



/**
 * @brief Test the negative scenarios for opening a video decoder.
 *
 * This test evaluates the behavior of the video decoder when attempting to open it with unsupported codecs, unsupported secure modes, and without meeting preconditions. It ensures that the decoder handles these scenarios gracefully and returns appropriate error statuses.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 012@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve secure mode from YAML configuration | securePathKey = "IVideoDecoderManager.IVideoDecoder.i.supportsSecure" | Secure mode value retrieved | Should be successful |
 * | 02 | Retrieve supported codecs from YAML configuration | codecCountKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs" | Supported codecs count retrieved | Should be successful |
 * | 03 | Test with unsupported codec | codec = 999, secure = secure, listener = listener, controller = nullptr | status.isOk() = true, controller = nullptr | Should Pass |
 * | 04 | Retrieve valid codec from YAML configuration | codecPathKey = "IVideoDecoderManager.IVideoDecoder.i.supportedCodecs.0", codecName = codecName | Valid codec value retrieved | Should be successful |
 * | 05 | Test with unsupported secure mode | codec = validCodec, secure = !secure, listener = listener, controller = nullptr | status.isOk() = true, controller = nullptr | Should Pass |
 * | 06 | Test without meeting precondition (not in CLOSED state) | codec = validCodec, secure = secure, listener = listener, controller = nullptr | status.isOk() = false, status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_negative_open) {
    for (size_t i = 0; i < decoders.size(); ++i) {
        sp<IVideoDecoderController> controller;
        sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();

        // Retrieve secure mode from YAML
        std::string securePathKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
        bool secure = UT_KVP_PROFILE_GET_BOOL(securePathKey.c_str());

        // Retrieve supported codecs from YAML
        std::string codecCountKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs";
        int codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(codecCountKey.c_str());


        // Test with unsupported codec
        Codec unsupportedCodec = static_cast<Codec>(999); // Assuming 999 is not a valid codec
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[])", static_cast<int>(unsupportedCodec), secure, listener.get());
        android::binder::Status status = decoders[i]->open(unsupportedCodec, secure, listener, &controller);
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int>(unsupportedCodec), secure, listener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(controller, nullptr);

        // Test with unsupported secure mode
        Codec validCodec;
        char codecName[UT_KVP_MAX_ELEMENT_SIZE];
        std::string codecPathKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs.0";
        UT_KVP_PROFILE_GET_STRING(codecPathKey.c_str(), codecName);
        if (strcmp(codecName, "MPEG") == 0) validCodec = Codec::MPEG;
        else if (strcmp(codecName, "H264") == 0) validCodec = Codec::H264;
        else if (strcmp(codecName, "H265") == 0) validCodec = Codec::H265;
        else if (strcmp(codecName, "VP9") == 0) validCodec = Codec::VP9;
        else if (strcmp(codecName, "AV1") == 0) validCodec = Codec::AV1;

        bool unsupportedSecure = !secure;
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[])", static_cast<int>(validCodec), unsupportedSecure, listener.get());
        status = decoders[i]->open(validCodec, unsupportedSecure, listener, &controller);
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int>(validCodec), unsupportedSecure, listener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(controller, nullptr);

        // Test without meeting precondition (not in CLOSED state)
        UT_LOG_INFO("Calling open with valid parameters for the first time");
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[])", static_cast<int>(validCodec), secure, listener.get());
        status = decoders[i]->open(validCodec, secure, listener, &controller);
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int>(validCodec), secure, listener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());

        UT_LOG_INFO("Calling open with valid parameters for the second time. Expecting current state is STARTED");
        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[])", static_cast<int>(validCodec), secure, listener.get());
        status = decoders[i]->open(validCodec, secure, listener, &controller);
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:listener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int>(validCodec), secure, listener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of closing a video decoder.
 *
 * This test verifies the functionality of the close API for video decoders. It ensures that the video decoder can be successfully closed after being started, and checks the return status and result of the close operation.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 013@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if VideoDecoderController is available | controllers = non-empty | Should be successful | |
 * | 02 | Start the video decoder to ensure it is in the READY state | controller = valid | status.isOk() = true | Should be successful |
 * | 03 | Log the call to the close API | controller = valid | Log entry created | Should be successful |
 * | 04 | Invoke the close API on the video decoder | controller = valid, result = false | status.isOk() = true, result = true | Should Pass |
 * | 05 | Log the result of the close API call | controller = valid, result = true | Log entry created | Should be successful |
 * | 06 | Verify the return value and output parameter of the close API | status.isOk() = true, result = true | status.isOk() = true, result = true | Should Pass |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_close) {
    bool result;
    for (size_t i = 0; i < decoders.size(); ++i) {

        sp<IVideoDecoderController> controller;
        sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();

        // Retrieve secure mode from YAML
        std::string securePathKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
        bool secure = UT_KVP_PROFILE_GET_BOOL(securePathKey.c_str());

        // Test with unsupported secure mode
        Codec validCodec;
        char codecName[UT_KVP_MAX_ELEMENT_SIZE];
        std::string codecPathKey = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs.0";
        UT_KVP_PROFILE_GET_STRING(codecPathKey.c_str(), codecName);
        if (strcmp(codecName, "MPEG") == 0) validCodec = Codec::MPEG;
        else if (strcmp(codecName, "H264") == 0) validCodec = Codec::H264;
        else if (strcmp(codecName, "H265") == 0) validCodec = Codec::H265;
        else if (strcmp(codecName, "VP9") == 0) validCodec = Codec::VP9;
        else if (strcmp(codecName, "AV1") == 0) validCodec = Codec::AV1;


        UT_LOG_INFO("Calling open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[])", static_cast<int32_t>(validCodec), secure, listener.get());
        android::binder::Status status = decoders[i]->open(validCodec, secure, listener, &controller);
        UT_LOG_INFO("Result open(IN:codec:[%d], IN:secure:[%d], IN:videoDecoderControllerListener:[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", static_cast<int32_t>(validCodec), secure, listener.get(), controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(controller != nullptr);

        // Log before calling the API
        UT_LOG_INFO("Calling close(IN:videoDecoderController[0x%X], OUT:_aidl_return)", controller.get());

        // Invoke the close API
        status = decoders[i]->close(controller, &result);

        // Log the result
        UT_LOG_INFO("Result close(IN:videoDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                    controller.get(), UT_Control_GetMapString(bool_mapTable, result),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Verify the return value and output parameter
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(result) << "Failed to close the video decoder.";
    }
}



/**
 * @brief Test the negative scenario of closing a VideoDecoder when not in READY state.
 *
 * This test verifies the behavior of the VideoDecoder's close function when the decoder is not in the READY state. It ensures that the function fails as expected, which is crucial for maintaining the integrity of the decoder's state management.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 014@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if VideoDecoderController is available | controllers = non-empty | Should be successful | |
 * | 02 | Log the call to close API | controller = VideoDecoderController instance | Should be successful | |
 * | 03 | Invoke the close API on VideoDecoderController | controller = VideoDecoderController instance, result = false | result = false, status.isOk() = false | Should Fail |
 * | 04 | Log the result after calling the API | controller = VideoDecoderController instance, result = false, status = failure | Should be successful | |
 * | 05 | Verify the result of close API | result = false | Expected close to fail | Should Fail |
 * | 06 | Verify the status of close API | status.isOk() = false | Expected status to indicate failure | Should Fail |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_negative_close) {
    for (size_t i = 0; i < decoders.size(); ++i) {
        bool result = false;
        sp<IVideoDecoderController> controller;
        // Log before calling the API
        UT_LOG_INFO("Calling close(IN:videoDecoderController, OUT:_aidl_return)");

        // Invoke the close API
        android::binder::Status status = decoders[i]->close(controller, &result);

        // Log the result after calling the API
        UT_LOG_INFO("Result close(IN:videoDecoderController[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                    controller.get(), UT_Control_GetMapString(bool_mapTable, result),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Verify the expected outputs
        UT_ASSERT_FALSE(result) << "Expected close to fail when not in READY state.";
        UT_ASSERT_FALSE(status.isOk()) << "Expected status to indicate failure.";
    }
}



/**
 * @brief Test the positive scenario of registering a video decoder event listener.
 *
 * This test verifies that the `registerEventListener` function successfully registers a video decoder event listener and returns the expected results. It ensures that the function call is successful and the listener is correctly registered.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 015@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a new instance of VideoDecoderEventListener. | listener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 02 | Iterate over each decoder in the decoders list. | decoders = list of decoders | Should iterate over all decoders | Should be successful |
 * | 03 | Call registerEventListener with the listener and check the status and aidl_return. | listener = VideoDecoderEventListener instance, aidl_return = false | status.isOk() should be true, aidl_return should be true | Should Pass |
 * | 04 | Log the result of registerEventListener call. | listener = VideoDecoderEventListener instance, aidl_return = result from call | Log should display the correct status and aidl_return | Should be successful |
 * | 05 | Assert that the status is OK and aidl_return is true. | status.isOk() = true, aidl_return = true | Assertions should pass | Should Pass |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_registerEventListener) {
    for (const auto& decoder : decoders) {
        sp<VideoDecoderEventListener> listener = new VideoDecoderEventListener();
        bool aidl_return = false;

        UT_LOG_INFO("Calling registerEventListener(IN:videoDecoderEventListener[0x%X], OUT:_aidl_return)", listener.get());
        android::binder::Status status = decoder->registerEventListener(listener, &aidl_return);
        UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                    listener.get(), UT_Control_GetMapString(bool_mapTable, aidl_return),
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(aidl_return);
    }
}



/**
 * @brief Test the negative scenario of registering the same event listener twice with a video decoder.
 *
 * This test checks the behavior of the video decoder when attempting to register the same event listener multiple times. The first registration should succeed, while the second should fail, ensuring that duplicate registrations are handled correctly.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 016@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a VideoDecoderEventListener instance | listener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 02 | Check if decoders list is not empty | decoders.empty() | Should be false | Should be successful |
 * | 03 | Retrieve the first decoder from the list | decoder = decoders.front() | Decoder instance should be retrieved successfully | Should be successful |
 * | 04 | Register the listener for the first time | listener = [0x%X], firstRegistrationResult = false | firstRegistrationResult = true, status.isOk() = true | Should Pass |
 * | 05 | Attempt to register the same listener again | listener = [0x%X], secondRegistrationResult = false | secondRegistrationResult = false, status.isOk() = true | Should Fail |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_negative_registerEventListener) {
    // Create a VideoDecoderEventListener instance
    sp<IVideoDecoderEventListener> listener = new VideoDecoderEventListener();

    // Assume the first decoder is used for testing
    UT_ASSERT_FALSE(decoders.empty()) << "No decoders available for testing.";
    sp<IVideoDecoder> decoder = decoders.front();

    // Register the listener for the first time
    bool firstRegistrationResult = false;
    UT_LOG_INFO("Calling registerEventListener(IN:videoDecoderEventListener:[0x%X], OUT:_aidl_return)", listener.get());
    android::binder::Status status = decoder->registerEventListener(listener, &firstRegistrationResult);
    UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener:[0x%X], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]",
                listener.get(), UT_Control_GetMapString(bool_mapTable, firstRegistrationResult),
                UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_TRUE(firstRegistrationResult);

    // Attempt to register the same listener again
    bool secondRegistrationResult = false;
    UT_LOG_INFO("Calling registerEventListener(IN:videoDecoderEventListener:[0x%X], OUT:_aidl_return)", listener.get());
    status = decoder->registerEventListener(listener, &secondRegistrationResult);
    UT_LOG_INFO("Result registerEventListener(IN:videoDecoderEventListener:[0x%X], OUT:_aidl_return:[%s]), status:[isOK:%s, exception:%s]",
                listener.get(), UT_Control_GetMapString(bool_mapTable, secondRegistrationResult),
                UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
    UT_ASSERT_TRUE(status.isOk());
    UT_ASSERT_FALSE(secondRegistrationResult);
}



/**
 * @brief Test the positive scenario of unregistering a VideoDecoder event listener.
 *
 * This test verifies the successful unregistration of an event listener from a VideoDecoder instance. It ensures that the event listener is correctly unregistered after being registered, which is crucial for managing event listeners and avoiding memory leaks or unwanted callbacks.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 017@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure that there are available VideoDecoder instances. | decoders.empty() = false | Assertion should pass | Should be successful |
 * | 02 | Create a new VideoDecoderEventListener instance. | videoDecoderEventListener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 03 | Register the event listener to the first VideoDecoder instance. | videoDecoderEventListener = [instance], registerResult = false | registerStatus.isOk() = true, registerResult = true | Should Pass |
 * | 04 | Log the call to unregister the event listener. | videoDecoderEventListener = [instance] | Log should be successful | Should be successful |
 * | 05 | Unregister the event listener from the first VideoDecoder instance. | videoDecoderEventListener = [instance], unregisterResult = false | unregisterStatus.isOk() = true, unregisterResult = true | Should Pass |
 * | 06 | Log the result of the unregister call. | videoDecoderEventListener = [instance], unregisterResult = [result], unregisterStatus = [status] | Log should be successful | Should be successful |
 * | 07 | Verify the unregistration status and result. | unregisterStatus.isOk() = true, unregisterResult = true | Assertion should pass | Should Pass |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_positive_unregisterEventListener) {
    UT_ASSERT_FALSE(decoders.empty()) << "No VideoDecoder instances available.";

    sp<IVideoDecoderEventListener> videoDecoderEventListener = new VideoDecoderEventListener();
    bool registerResult = false;
    UT_LOG_INFO("Calling registerEventListener as prerequisite");
    android::binder::Status registerStatus = decoders[0]->registerEventListener(videoDecoderEventListener, &registerResult);
    UT_ASSERT_TRUE(registerStatus.isOk()) << "Failed to register event listener.";
    UT_ASSERT_TRUE(registerResult) << "Event listener registration unsuccessful.";

    UT_LOG_INFO("Calling unregisterEventListener(IN:videoDecoderEventListener[0x%X], OUT:_aidl_return)", videoDecoderEventListener.get());
    bool unregisterResult = false;
    android::binder::Status unregisterStatus = decoders[0]->unregisterEventListener(videoDecoderEventListener, &unregisterResult);

    UT_LOG_INFO("Result unregisterEventListener(IN:videoDecoderEventListener[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]",
                videoDecoderEventListener.get(), UT_Control_GetMapString(bool_mapTable, unregisterResult),
                UT_Control_GetMapString(bool_mapTable, unregisterStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, unregisterStatus.exceptionCode()));

    UT_ASSERT_TRUE(unregisterStatus.isOk()) << "Failed to unregister event listener.";
    UT_ASSERT_TRUE(unregisterResult) << "Event listener unregistration unsuccessful.";
}



/**
 * @brief Test the negative scenario of unregistering a video decoder event listener
 *
 * This test verifies the behavior of the `unregisterEventListener` function when attempting to unregister a video decoder event listener that has not been registered. The test ensures that the function handles this scenario gracefully without causing any errors or unexpected behavior.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 018@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Create a new instance of VideoDecoderEventListener | videoDecoderEventListener = new VideoDecoderEventListener() | Instance should be created successfully | Should be successful |
 * | 02 | Ensure the videoDecoderEventListener has not been registered | N/A | No prior registration should exist | Should be successful |
 * | 03 | Log the API call for unregisterEventListener | IN: videoDecoderEventListener, OUT: aidl_return | Log should be generated | Should be successful |
 * | 04 | Call the unregisterEventListener API | IN: videoDecoderEventListener, OUT: aidl_return | aidl_return should be false, status should be OK | Should Fail |
 * | 05 | Verify the return value and output parameter | aidl_return = false, status.isOk() = true | aidl_return should be false, status should be OK | Should be successful |
 * | 06 | Log the result of the API call | IN: videoDecoderEventListener, OUT: aidl_return, status | Log should be generated | Should be successful |
 */
UT_ADD_TEST(VideoDecoderBaseTest, test_l1_VideoDecoder_negative_unregisterEventListener) {
    sp<IVideoDecoderEventListener> videoDecoderEventListener = new VideoDecoderEventListener();
    bool aidl_return = false;

    // Ensure the videoDecoderEventListener has not been registered
    // (Assuming no prior registration code is present)

    // Log the API call
    UT_LOG_INFO("Calling unregisterEventListener() without prior registerEventListener()");
    UT_LOG_INFO("Calling Calling(IN:videoDecoderEventListener[0x%X], OUT:aidl_return)", videoDecoderEventListener.get());

    // Call the API
    android::binder::Status status = decoders[0]->unregisterEventListener(videoDecoderEventListener, &aidl_return);

    // Verify the return value and output parameter
    UT_ASSERT_FALSE(aidl_return);
    UT_ASSERT_TRUE(status.isOk());

    // Log the result
    UT_LOG_INFO("Result unregisterEventListener(IN:videoDecoderEventListener[0x%X], OUT:aidl_return[%s]), status:[isOK:%s, exception:%s]",
                videoDecoderEventListener.get(),
                UT_Control_GetMapString(bool_mapTable, aidl_return),
                UT_Control_GetMapString(bool_mapTable, status.isOk()),
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
}



/**
 * @brief Test the positive scenario of starting the Video Decoder Controller
 *
 * This test verifies that the Video Decoder Controller can be successfully started when it is in the READY state. It ensures that the start API is called correctly and that the controller transitions to the expected state without errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 019@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve the current state of the Video Decoder to ensure it is READY | controller = <controller_instance>, currentState = <state_variable> | currentState should be READY | Should be successful |
 * | 02 | Log the action of calling the start API | None | Log should indicate "Calling start()" | Should be successful |
 * | 03 | Call the start API on the Video Decoder Controller | controller = <controller_instance> | status.isOk() should be true | Should Pass |
 * | 04 | Log the result of the start API call | status = <status_variable> | Log should indicate the status of the start call | Should be successful |
 * | 05 | Verify the return value of the start API indicates success | status = <status_variable> | status.isOk() should be true | Should Pass |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_positive_start) {
    for (const auto& decoder : decoders) {
        ::com::rdk::hal::State currentState;
        android::binder::Status status = decoder->getState(&currentState);
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(currentState, ::com::rdk::hal::State::READY);
    }

    for (const auto& controller : controllers) {
        // Log before calling the start API
        UT_LOG_INFO("Calling start()");
        // Call the start API
        android::binder::Status status = controller->start();
        // Log the result of the start API call
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        // Verify the return value indicates success
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder controller ";
    }
}



/**
 * @brief Test the behavior of VideoDecoderController when attempting to start an already started decoder.
 *
 * This test verifies that the VideoDecoderController correctly handles an attempt to start the video decoder
 * when it is already in the STARTED state. The expected behavior is that the start operation should fail
 * with an EX_ILLEGAL_STATE exception, indicating that the decoder is already started.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 020@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the Video Decoder is in the STARTED state by calling start() as a prerequisite. | controller = instance, status = result of start() | status.isOk() should be true | Should be successful |
 * | 02 | Attempt to start the Video Decoder again while it is already in the STARTED state. | controller = instance, status = result of start() | status.isOk() should be false, status.exceptionCode() should be EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_start) {
    for (const auto& controller : controllers) {
        // Ensure the Video Decoder is in the STARTED state by calling start() as prerequisite.
        android::binder::Status status = controller->start();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder for prerequisite setup.";

        // Now the Video Decoder is in the STARTED state, attempt to start again.
        UT_LOG_INFO("Calling start()");
        status = controller->start();
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Verify the return value is EX_ILLEGAL_STATE.
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder controller ";
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of stopping the Video Decoder Controller
 *
 * This test verifies that the Video Decoder Controller can be successfully stopped after being started.
 * It ensures that the stop function works correctly when the controller is in the STARTED state.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 021@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Start the Video Decoder Controller | controller = STARTED | Controller should start successfully | Should be successful |
 * | 02 | Log the call to stop the controller | None | Log should be recorded | Should be successful |
 * | 03 | Call the stop API on the Video Decoder Controller | controller = STOPPED | Controller should stop successfully | Should Pass |
 * | 04 | Log the result of the stop call | None | Log should be recorded | Should be successful |
 * | 05 | Verify the stop call was successful | status.isOk() = true | Assertion should pass | Should Pass |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_positive_stop) {
    for (const auto& controller : controllers) {
        // Ensure the controller is in the STARTED state
        android::binder::Status status = controller->start();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder Controller";

        // Log the call to stop
        UT_LOG_INFO("Calling stop()");

        // Call the stop API
        status = controller->stop();

        // Log the result of the stop call
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Verify the stop call was successful
        UT_ASSERT_TRUE(status.isOk()) << "Failed to stop the Video Decoder Controller";
    }
}



/**
 * @brief Test the negative scenario of stopping the VideoDecoderController when it is not in the STARTED state.
 *
 * This test verifies that the stop() function of the VideoDecoderController returns an EX_ILLEGAL_STATE exception when the controller is not in the STARTED state. This ensures that the stop() function behaves correctly under invalid conditions.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 022@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the decoder is not in the STARTED state. | N/A | N/A | Should be successful |
 * | 02 | Log the action before calling the stop API. | N/A | N/A | Should be successful |
 * | 03 | Call the stop() API on the VideoDecoderController. | controller = instance, method = stop() | status.exceptionCode() = EX_ILLEGAL_STATE | Should Pass |
 * | 04 | Log the result of the stop API call. | N/A | N/A | Should be successful |
 * | 05 | Verify that the stop API returns EX_ILLEGAL_STATE. | status.exceptionCode() = EX_ILLEGAL_STATE | Assertion should pass | Should Pass |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_stop) {
    for (const auto& controller : controllers) {
        // Ensure the decoder is not in the STARTED state
        // Assuming the decoder is in READY state after setup
        android::binder::Status status;
        // Log before calling the stop API
        UT_LOG_INFO("Calling stop()");
        // Call the stop API
        status = controller->stop();
        // Log the result of the stop API
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        // Verify that the stop API returns EX_ILLEGAL_STATE
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenarios for setting properties on the VideoDecoderController.
 *
 * This test verifies that various properties can be successfully set on the VideoDecoderController when it is in the READY state. 
 * It ensures that the setProperty function behaves as expected for different property values.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 023@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the decoder is in the READY state | controller = current controller, currentState = READY | currentState should be READY | Should be successful |
 * | 02 | Set Operational Mode to NON_TUNNELLED | property = OPERATIONAL_MODE, propertyValue = NON_TUNNELLED, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 03 | Set Low Latency Mode to ON | property = LOW_LATENCY_MODE, propertyValue = ON, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 04 | Set Decode Error Policy to Conceal Corrupt Frames | property = DECODE_ERROR_POLICY, propertyValue = Conceal Corrupt Frames, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 05 | Set AV Source to AVUNKNOWN | property = AV_SOURCE, propertyValue = AVUNKNOWN, result = false | status.isOk() should be true, result should be true | Should Pass |
 * | 06 | Set SHA1 Calculation to ON | property = SHA1_CALC, propertyValue = ON, result = false | status.isOk() should be true, result should be true | Should Pass |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_positive_setProperty) {
    // Ensure the decoder is in the READY state
    for (const auto& decoder : decoders) {
            ::com::rdk::hal::State currentState;
            android::binder::Status status = decoder->getState(&currentState);
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, ::com::rdk::hal::State::READY);
    }

    for (const auto& controller : controllers) {

        bool result = false;

        // Test Case 1: Set Operational Mode to NON_TUNNELLED
        {
            ::com::rdk::hal::PropertyValue propertyValue;

            char operationalmode[UT_KVP_MAX_ELEMENT_SIZE];
            std::string operationalmode_path = "IVideoDecoderManager.supportedOperationalModes." + std::to_string(0);
            UT_KVP_PROFILE_GET_STRING(operationalmode_path.c_str(), operationalmode);
            OperationalMode  opmode = stringToOperationalMode(operationalmode);

            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(static_cast<int32_t>(opmode)); // NON_TUNNELLED
            UT_LOG_INFO("Calling setProperty(IN:property:[OPERATIONAL_MODE], IN:propertyValue:[NON_TUNNELLED])");
            android::binder::Status status = controller->setProperty(::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE, propertyValue, &result);
            UT_LOG_INFO("Result setProperty(IN:property:[OPERATIONAL_MODE], IN:propertyValue:[NON_TUNNELLED]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(result);
        }

        // Test Case 2: Set Low Latency Mode to ON
        {
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(1); // ON
            UT_LOG_INFO("Calling setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[ON])");
            android::binder::Status status = controller->setProperty(::com::rdk::hal::videodecoder::Property::LOW_LATENCY_MODE, propertyValue, &result);
            UT_LOG_INFO("Result setProperty(IN:property:[LOW_LATENCY_MODE], IN:propertyValue:[ON]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(result);
        }

        // Test Case 3: Set Decode Error Policy to Conceal Corrupt Frames
        {
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(1); // Conceal Corrupt Frames
            UT_LOG_INFO("Calling setProperty(IN:property:[DECODE_ERROR_POLICY], IN:propertyValue:[Conceal Corrupt Frames])");
            android::binder::Status status = controller->setProperty(::com::rdk::hal::videodecoder::Property::DECODE_ERROR_POLICY, propertyValue, &result);
            UT_LOG_INFO("Result setProperty(IN:property:[DECODE_ERROR_POLICY], IN:propertyValue:[Conceal Corrupt Frames]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(result);
        }

        // Test Case 4: Set AV Source to AVUNKNOWN
        {
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(0); // AVUNKNOWN
            UT_LOG_INFO("Calling setProperty(IN:property:[AV_SOURCE], IN:propertyValue:[AVUNKNOWN])");
            android::binder::Status status = controller->setProperty(::com::rdk::hal::videodecoder::Property::AV_SOURCE, propertyValue, &result);
            UT_LOG_INFO("Result setProperty(IN:property:[AV_SOURCE], IN:propertyValue:[AVUNKNOWN]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(result);
        }

        // Test Case 5: Set SHA1 Calculation to ON
        {
            ::com::rdk::hal::PropertyValue propertyValue;
            propertyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(1); // ON
            UT_LOG_INFO("Calling setProperty(IN:property:[SHA1_CALC], IN:propertyValue:[ON])");
            android::binder::Status status = controller->setProperty(::com::rdk::hal::videodecoder::Property::SHA1_CALC, propertyValue, &result);
            UT_LOG_INFO("Result setProperty(IN:property:[SHA1_CALC], IN:propertyValue:[ON]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(result);
        }
    }
}



/**
 * @brief Test the negative scenarios for setting properties in VideoDecoderController
 *
 * This test evaluates the behavior of the VideoDecoderController when attempting to set properties with invalid or unsupported values. It ensures that the controller correctly handles these scenarios by not allowing the property to be set and returning the appropriate status.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 024@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Check if VideoDecoderController is available for testing | controllers.empty() | Should be false | Should be successful |
 * | 02 | Set Operational Mode to an Unsupported Mode | property = OPERATIONAL_MODE, propertyValue = 999 | result = false, status.isOk() = false | Should Fail |
 * | 03 | Set Property in Invalid State | property = LOW_LATENCY_MODE, propertyValue = 1 | result = false, status.isOk() = false | Should Fail |
 * | 04 | Set Property with Invalid Value Type | property = DECODE_ERROR_POLICY, propertyValue = "invalid" | result = false, status.isOk() = false | Should Fail |
 * | 05 | Set Property with Out-of-Range Value | property = SHA1_CALC, propertyValue = 999 | result = false, status.isOk() = false | Should Fail |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_setProperty) {
    UT_ASSERT_FALSE(controllers.empty()) << "No VideoDecoderController available for testing.";

    for (const auto& controller : controllers) {
        bool result = false;

        // Test Case: Set Operational Mode to an Unsupported Mode
        UT_LOG_INFO("Calling setProperty(IN:property[OPERATIONAL_MODE], IN:propertyValue[unsupported mode])");
        ::com::rdk::hal::PropertyValue unsupportedModeValue;
        unsupportedModeValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(999); // Unsupported mode
        android::binder::Status status = controller->setProperty(::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE, unsupportedModeValue, &result);
        UT_LOG_INFO("Result setProperty(IN:property[OPERATIONAL_MODE], IN:propertyValue[unsupported mode]), status:[isOK:%s, exception:%s], result:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), UT_Control_GetMapString(bool_mapTable, result));
        UT_ASSERT_FALSE(result);

        // Test Case: Set Property in Invalid State (STARTED)
        status = controller->start();
        UT_LOG_INFO("Calling setProperty(IN:property[LOW_LATENCY_MODE], IN:propertyValue[1])");
        ::com::rdk::hal::PropertyValue lowLatencyValue;
        lowLatencyValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(1); // ON
        status = controller->setProperty(::com::rdk::hal::videodecoder::Property::LOW_LATENCY_MODE, lowLatencyValue, &result);
        status = controller->stop();
        UT_LOG_INFO("Result setProperty(IN:property[LOW_LATENCY_MODE], IN:propertyValue[1]), status:[isOK:%s, exception:%s], result:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), UT_Control_GetMapString(bool_mapTable, result));
        UT_ASSERT_FALSE(result);

        // Test Case: Set Property with Invalid Value Type
        UT_LOG_INFO("Calling setProperty(IN:property[DECODE_ERROR_POLICY], IN:propertyValue[string])");
        ::com::rdk::hal::PropertyValue invalidTypeValue;
        invalidTypeValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::stringValue>(android::String16("invalid"));
        status = controller->setProperty(::com::rdk::hal::videodecoder::Property::DECODE_ERROR_POLICY, invalidTypeValue, &result);
        UT_LOG_INFO("Result setProperty(IN:property[DECODE_ERROR_POLICY], IN:propertyValue[string]), status:[isOK:%s, exception:%s], result:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), UT_Control_GetMapString(bool_mapTable, result));
        UT_ASSERT_FALSE(result);

        // Test Case: Set Property with Out-of-Range Value
        UT_LOG_INFO("Calling setProperty(IN:property[SHA1_CALC], IN:propertyValue[999])");
        ::com::rdk::hal::PropertyValue outOfRangeValue;
        outOfRangeValue.value = ::com::rdk::hal::PropertyValue::Value::make<::com::rdk::hal::PropertyValue::Value::Tag::intValue>(999); // Out of expected range
        status = controller->setProperty(::com::rdk::hal::videodecoder::Property::SHA1_CALC, outOfRangeValue, &result);
        UT_LOG_INFO("Result setProperty(IN:property[SHA1_CALC], IN:propertyValue[999]), status:[isOK:%s, exception:%s], result:[%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()), UT_Control_GetMapString(bool_mapTable, result));
        UT_ASSERT_FALSE(result);
    }
}



/**
 * @brief Test the negative scenarios for the decodeBuffer function in VideoDecoderController
 *
 * This test evaluates the behavior of the decodeBuffer function when provided with invalid inputs or states. It ensures that the function handles these scenarios gracefully by returning appropriate error codes and results.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 025@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Attempt to decode buffer without starting the decoder | nsPresentationTime = 3000000, bufferHandle = 1, result = false | result = false, exceptionCode = EX_ILLEGAL_STATE | Should Fail |
 * | 02 | Attempt to decode buffer with an invalid buffer handle | nsPresentationTime = 4000000, bufferHandle = -1, result = false | result = false, exceptionCode = EX_ILLEGAL_ARGUMENT | Should Fail |
 * | 03 | Attempt to decode buffer with an invalid presentation time | nsPresentationTime = -1000000, bufferHandle = 1, result = false | result = false, exceptionCode = EX_ILLEGAL_ARGUMENT | Should Fail |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_decodeBuffer) {
    for (const auto& controller : controllers) {
        bool result = false;
        android::binder::Status status;

        // Test Case 3: Invalid State - Decoder Not Started
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[3000000], IN:bufferHandle:[1], OUT:_aidl_return[])");
        status = controller->decodeBuffer(3000000, 1, &result);
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[3000000], IN:bufferHandle:[1], OUT:_aidl_return[%d]), status:[isOK:%s, exception:%s]", result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(result);
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);

        //changing state to STARTED as prerquisite
        status = controller->start();

        // Test Case 4: Invalid Buffer Handle
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[4000000], IN:bufferHandle:[-1], OUT:_aidl_return[])");
        status = controller->decodeBuffer(4000000, -1, &result);
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[4000000], IN:bufferHandle:[-1], OUT:_aidl_return[%d]), status:[isOK:%s, exception:%s]", result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(result);
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_ARGUMENT);

        // Test Case 5: Invalid nsPresentationTime
        UT_LOG_INFO("Calling decodeBuffer(IN:nsPresentationTime:[-1000000], IN:bufferHandle:[1], OUT:_aidl_return[])");
        status = controller->decodeBuffer(-1000000, 1, &result);
        UT_LOG_INFO("Result decodeBuffer(IN:nsPresentationTime:[-1000000], IN:bufferHandle:[1], OUT:_aidl_return[%d]), status:[isOK:%s, exception:%s]", result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(result);
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_ARGUMENT);

        //Reverting state to READY
        status = controller->stop();
    }
}



/**
 * @brief Test the flush functionality of the VideoDecoderController with different reset values.
 *
 * This test verifies the behavior of the flush method in the VideoDecoderController when invoked with different reset values. It ensures that the decoder transitions to the correct states and handles the flush operation correctly.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 026@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the decoder and controller objects | controllers, openedDecoders | Should be successful | |
 * | 02 | Start the decoder to ensure it is in the STARTED state | controller, started = false | Decoder should start successfully | Should be successful |
 * | 03 | Flush the decoder with reset = true | reset = true | Flush should succeed, decoder should be in READY state | Should Pass |
 * | 04 | Stop the decoder to ensure it is in the READY state | controller, ready = false | Decoder should stop successfully | Should be successful |
 * | 05 | Start the decoder again to ensure it is in the STARTED state | controller, started = false | Decoder should start successfully | Should be successful |
 * | 06 | Flush the decoder with reset = false | reset = false | Flush should succeed, decoder should remain in STARTED state | Should Pass |
 * | 07 | Stop the decoder to ensure it remains in the STARTED state | controller, ready = false | Decoder should stop successfully | Should be successful |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_positive_flush) {
    for (size_t i = 0; i < controllers.size(); ++i) {
        sp<IVideoDecoderController> controller = controllers[i];
        // Ensure the decoder is in the STARTED state
        bool started = false;
        android::binder::Status status;
        UT_LOG_INFO("Calling start()");
        status = controller->start();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder controller ";

        // Test Case 1: Flush with reset = true
        UT_LOG_INFO("Calling flush(IN:reset:[true])");
        status = controller->flush(true);
        UT_LOG_INFO("Result flush(IN:reset:[true]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());

        UT_LOG_INFO("Calling flush(IN:reset:[false])");
        status = controller->flush(false);
        UT_LOG_INFO("Result flush(IN:reset:[false]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());

        // Ensure the decoder remains in the READY state
        status = controller->stop();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to stop the Video Decoder controller";
    }
}



/**
 * @brief Test the negative flush operation of the VideoDecoderController
 *
 * This test verifies the behavior of the flush operation when the video decoder controller is not in the STARTED state.
 * It ensures that the flush operation returns an illegal state exception when invoked in an inappropriate state.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 027@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the decoder is in a state other than STARTED by stopping it | controller = instance, result = false | status.isOk() = true, result = true | Should be successful |
 * | 02 | Call the flush API with reset = true | reset = true | status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 * | 03 | Call the flush API with reset = false | reset = false | status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_flush) {
    for (const auto& controller : controllers) {
        // Ensure the decoder is in a state other than STARTED (e.g., READY)
        // Call the flush API with reset = true
        android::binder::Status status;
        UT_LOG_INFO("Calling flush(IN:reset:[true])");
        status = controller->flush(true);
        UT_LOG_INFO("Result flush(IN:reset:[true]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);

        // Call the flush API with reset = false
        UT_LOG_INFO("Calling flush(IN:reset:[false])");
        status = controller->flush(false);
        UT_LOG_INFO("Result flush(IN:reset:[false]), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of signaling discontinuity in VideoDecoderController
 *
 * This test verifies the behavior of the VideoDecoderController when signaling a discontinuity in the STARTED state. It ensures that the controller transitions correctly between states and handles the signalDiscontinuity call without errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 028@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the VideoDecoderController and ensure it is in the READY state by calling start() | controller = initialized, status = uninitialized | status.isOk() = true | Should be successful |
 * | 02 | Call signalDiscontinuity() to test the transition in the STARTED state | controller = started, status = uninitialized | status.isOk() = true | Should Pass |
 * | 03 | Call stop() to transition back to the READY state | controller = started, status = uninitialized | status.isOk() = true | Should be successful |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_positive_signalDiscontinuity) {
    for (const auto& controller : controllers) {
        // Ensure the Video Decoder is in the STARTED state
        android::binder::Status status;
        bool result = false;

        // Transition to READY state
        UT_LOG_INFO("Calling start()");
        status = controller->start();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder";
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Transition to STARTED state
        UT_LOG_INFO("Calling signalDiscontinuity()");
        status = controller->signalDiscontinuity();
        UT_ASSERT_TRUE(status.isOk()) << "signalDiscontinuity failed in STARTED state";
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Transition back to READY state
        UT_LOG_INFO("Calling stop()");
        status = controller->stop();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to stop the Video Decoder";
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
    }
}



/**
 * @brief Test the negative scenario for the signalDiscontinuity API in VideoDecoderController
 *
 * This test verifies that the signalDiscontinuity API returns an error status when the Video Decoder is not in the STARTED state. The expected error is EX_ILLEGAL_STATE.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 029@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the Video Decoder is in a state other than STARTED by calling the stop function | controller = instance, result = false | status.isOk() = true, result = true | Should be successful |
 * | 02 | Call the signalDiscontinuity API on the controller | controller = instance | status.isOk() = false, status.exceptionCode() = EX_ILLEGAL_STATE | Should Fail |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_signalDiscontinuity) {
    android::binder::Status status;
    for (const auto& controller : controllers) {
        // Call the signalDiscontinuity API
        UT_LOG_INFO("Calling signalDiscontinuity()");
        status = controller->signalDiscontinuity();

        // Verify that the API call returns an error status, specifically EX_ILLEGAL_STATE
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_FALSE(status.isOk());
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenario of signaling EOS in VideoDecoderController
 *
 * This test verifies that the VideoDecoderController can successfully signal the end of stream (EOS) when it is in the STARTED state. It ensures that the controller transitions correctly and that the signalEOS function behaves as expected.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 030@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the Video Decoder is in the STARTED state by calling start() | controller = <controller_instance>, started = false | status.isOk() = true, started = true | Should be successful |
 * | 02 | Log before calling signalEOS | None | None | Should be successful |
 * | 03 | Call signalEOS on the Video Decoder Controller | controller = <controller_instance> | status.isOk() = true | Should Pass |
 * | 04 | Log the result of signalEOS | None | None | Should be successful |
 * | 05 | Verify the return status of signalEOS is successful | status.isOk() = true | status.isOk() = true | Should Pass |
 * | 06 | Stop the Video Decoder as teardown by calling stop() | controller = <controller_instance>, stopped = false | status.isOk() = true, stopped = true | Done by Pre-requisite TearDown function |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_positive_signalEOS) {
    for (const auto& controller : controllers) {
        // Ensure the Video Decoder is in the STARTED state
        android::binder::Status status = controller->start();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder controller ";

        // Log before calling signalEOS
        UT_LOG_INFO("Calling signalEOS()");

        // Call signalEOS
        status = controller->signalEOS();

        // Log the result of signalEOS
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));

        // Verify the return status of signalEOS is successful
        UT_ASSERT_TRUE(status.isOk());

        // Stop the Video Decoder as teardown
        status = controller->stop();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to stop the Video Decoder controller ";
    }
}



/**
 * @brief Test the behavior of VideoDecoderController when signalEOS is called in an invalid state.
 *
 * This test verifies that the VideoDecoderController correctly handles the signalEOS call when the decoder is not in the STARTED state. It ensures that the function returns an EX_ILLEGAL_STATE exception, indicating that the operation is not allowed in the current state.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 031@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Retrieve the current state of the Video Decoder to ensure it is not STARTED. | controller = instance, currentState = READY | currentState != STARTED | Should be successful |
 * | 02 | Call signalEOS() on the Video Decoder. | controller = instance | status.exceptionCode() = EX_ILLEGAL_STATE | Should Pass |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_signalEOS) {
    for (const auto& controller : controllers) {
        // Call signalEOS() on the Video Decoder
        UT_LOG_INFO("Calling signalEOS()");
        android::binder::Status status = controller->signalEOS();

        // Verify the return status of signalEOS() is EX_ILLEGAL_STATE
        UT_LOG_INFO("Result signalEOS(), status:[isOK:%s, exception:%s]",
                    UT_Control_GetMapString(bool_mapTable, status.isOk()),
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_EQUAL(status.exceptionCode(), android::binder::Status::EX_ILLEGAL_STATE);
    }
}



/**
 * @brief Test the positive scenarios for parsing codec specific data in VideoDecoderController.
 *
 * This test verifies that the VideoDecoderController can correctly parse codec specific data for different video formats such as AVC, HEVC, and AV1. It ensures that the controller is able to start, process the codec data, and stop without errors.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 032@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Ensure the VideoDecoderController is not empty and available for testing. | controllers | Should be successful | |
 * | 02 | Start the VideoDecoderController to bring it to the STARTED state. | result = false | Should Pass | |
 * | 03 | Parse codec specific data for AVC_DECODER_CONFIGURATION_RECORD. | csdVideoFormat = AVC_DECODER_CONFIGURATION_RECORD, codecData = 0x01, _aidl_return = false | Should Pass | Should Pass |
 * | 04 | Parse codec specific data for HEVC_DECODER_CONFIGURATION_RECORD. | csdVideoFormat = HEVC_DECODER_CONFIGURATION_RECORD, codecData = 0x01, _aidl_return = false | Should Pass | Should Pass |
 * | 05 | Parse codec specific data for AV1_DECODER_CONFIGURATION_RECORD. | csdVideoFormat = AV1_DECODER_CONFIGURATION_RECORD, codecData = 0x01, _aidl_return = false | Should Pass | Should Pass |
 * | 06 | Stop the VideoDecoderController after tests. | result = false | Should Pass | |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_positive_parseCodecSpecificData) {
    UT_ASSERT_FALSE(controllers.empty()) << "No VideoDecoderController available for testing.";
    Status status;

    for (const auto& controller : controllers) {
        // Ensure the Video Decoder is in the STARTED state
        bool result = false;
       status = controller->start();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder controller ";

        // Test for AVC_DECODER_CONFIGURATION_RECORD
        {
            bool _aidl_return = false;
            UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[AVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[Valid AVCDecoderConfigurationRecord])");
            status = controller->parseCodecSpecificData(
                ::com::rdk::hal::videodecoder::CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD,
                /* codecData: Valid byte array representing AVCDecoderConfigurationRecord */ 0x01, // Example byte
                &_aidl_return
            );
            UT_LOG_INFO("Result parseCodecSpecificData(IN:csdVideoFormat:[AVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[Valid AVCDecoderConfigurationRecord]), status:[isOK:%s, exception:%s], OUT:_aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(_aidl_return);
        }

        // Test for HEVC_DECODER_CONFIGURATION_RECORD
        {
            bool _aidl_return = false;
            UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[HEVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[Valid HEVCDecoderConfigurationRecord])");
            status = controller->parseCodecSpecificData(
                ::com::rdk::hal::videodecoder::CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD,
                /* codecData: Valid byte array representing HEVCDecoderConfigurationRecord */ 0x01, // Example byte
                &_aidl_return
            );
            UT_LOG_INFO("Result parseCodecSpecificData(IN:csdVideoFormat:[HEVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[Valid HEVCDecoderConfigurationRecord]), status:[isOK:%s, exception:%s], OUT:_aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(_aidl_return);
        }

        // Test for AV1_DECODER_CONFIGURATION_RECORD
        {
            bool _aidl_return = false;
            UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[AV1_DECODER_CONFIGURATION_RECORD], IN:codecData:[Valid AV1CodecConfigurationRecord])");
            status = controller->parseCodecSpecificData(
                ::com::rdk::hal::videodecoder::CSDVideoFormat::AV1_DECODER_CONFIGURATION_RECORD,
                /* codecData: Valid byte array representing AV1CodecConfigurationRecord */ 0x01, // Example byte
                &_aidl_return
            );
            UT_LOG_INFO("Result parseCodecSpecificData(IN:csdVideoFormat:[AV1_DECODER_CONFIGURATION_RECORD], IN:codecData:[Valid AV1CodecConfigurationRecord]), status:[isOK:%s, exception:%s], OUT:_aidl_return:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, _aidl_return));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(_aidl_return);
        }

        // Stop the Video Decoder after tests
        status = controller->stop();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to stop the VideoDecoderController.";
    }
}



/**
 * @brief Test the negative scenarios for parsing codec specific data in VideoDecoderController.
 *
 * This test evaluates the behavior of the VideoDecoderController when parsing codec specific data with invalid inputs or states. It ensures that the function handles these scenarios gracefully without causing unexpected behavior.
 *
 * **Test Group ID:** Basic: 01@n
 * **Test Case ID:** 033@n
 * **Priority:** High@n
 * @n
 * **Pre-Conditions:** None@n
 * **Dependencies:** None@n
 * **User Interaction:** None@n
 * @n
 * **Test Procedure:**@n
 * | Variation / Step | Description | Test Data | Expected Result | Notes |
 * | :----: | --------- | ---------- |-------------- | ----- |
 * | 01 | Initialize the test environment and iterate over each controller. | controllers = list of controllers | Should be successful | |
 * | 02 | Call parseCodecSpecificData with empty codec data. | csdVideoFormat = AVC_DECODER_CONFIGURATION_RECORD, codecData = empty, result = false | result = false | Should Fail |
 * | 03 | Simulate the decoder not being in the STARTED state by calling stop. | isStarted = false | status.isOk() = true | Should be successful |
 * | 04 | Call parseCodecSpecificData with valid HEVC data while decoder is not started. | csdVideoFormat = HEVC_DECODER_CONFIGURATION_RECORD, codecData = validHEVCData, result = false | result = false | Should Fail |
 * | 05 | Ensure the decoder is in the STARTED state by calling start. | isStarted = false | status.isOk() = true | Should be successful |
 * | 06 | Call parseCodecSpecificData with mismatched csdVideoFormat. | csdVideoFormat = invalid, codecData = validData, result = false | result = false | Should Fail |
 */
UT_ADD_TEST(VideoDecoderControllerBaseTest, test_l1_VideoDecoderController_negative_parseCodecSpecificData) {
    ::android::binder::Status status;
    for (const auto& controller : controllers) {
        bool result = false;

        // Negative Test with Invalid State , ie, Not in STARTED state
        {
            std::vector<uint8_t> validHEVCData = {0x01, 0x02, 0x03}; // Example valid data
            UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[HEVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[validHEVCData])");
            status = controller->parseCodecSpecificData(
                ::com::rdk::hal::videodecoder::CSDVideoFormat::HEVC_DECODER_CONFIGURATION_RECORD, 0x01,&result);
            UT_LOG_INFO("Result parseCodecSpecificData(IN:csdVideoFormat:[HEVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[validHEVCData]), status:[isOK:%s, exception:%s], result:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, result));
            UT_ASSERT_FALSE(result);
        }

        // state change to STARTED as prerequisite
        status = controller->start();
        UT_ASSERT_TRUE(status.isOk()) << "Failed to start the Video Decoder controller ";
        // Negative Test with Empty Codec Data
        {
            std::vector<uint8_t> emptyHEVCData = {};
            UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[AVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[empty])");
            status = controller->parseCodecSpecificData(
                ::com::rdk::hal::videodecoder::CSDVideoFormat::AVC_DECODER_CONFIGURATION_RECORD,
                0x01, // Empty codec data
                &result);
            UT_LOG_INFO("Result parseCodecSpecificData(IN:csdVideoFormat:[AVC_DECODER_CONFIGURATION_RECORD], IN:codecData:[empty]), status:[isOK:%s, exception:%s], result:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, result));
            UT_ASSERT_FALSE(result);
        }


        // Negative Test with Mismatched csdVideoFormat
        {
            std::vector<int8_t> validData = {0x01, 0x02, 0x03}; // Example valid data
            UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdVideoFormat:[invalid], IN:codecData:[validData])");
            status = controller->parseCodecSpecificData(
                static_cast<::com::rdk::hal::videodecoder::CSDVideoFormat>(-1), // Invalid format
                0x01,
                &result
            );
            UT_LOG_INFO("Result parseCodecSpecificData(IN:csdVideoFormat:[invalid], IN:codecData:[validData]), status:[isOK:%s, exception:%s], result:[%s]",
                        UT_Control_GetMapString(bool_mapTable, status.isOk()),
                        UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()),
                        UT_Control_GetMapString(bool_mapTable, result));
            UT_ASSERT_FALSE(result);
        }

        status = controller->stop();
        UT_ASSERT_TRUE(status.isOk());
    }
}


