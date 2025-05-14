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
 * @defgroup Video_Decoder_Halif_Test_L2 Video Decoder HALIF Tests L2 File
 * @{
 * @parblock
 *
 * ### L2 Tests for Video Decoder HALIF AIDL :
 *
 * Level 2 test cases for all APIs of Video Decoder HALIF AIDL Interface
 *
 * **Pre-Conditions:**  None@n
 * **Dependencies:** None@n
 *
 * Ref to API Definition specification documentation :
 *
 * @endparblock
 */


/**
 * @file vts_videodecoder_l2_tests.cpp
 *
 */

 #include "vts_videodecoder.h"



/**
* @brief Test the ability to set and get properties on video decoders
*
* This test verifies that the video decoders are correctly instantiated and their properties can be set and retrieved. It ensures that the decoder is not null, checks the setting and getting of read-write properties, and validates the closing of the decoder.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 001@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_High-Level_TestSpec.md](../../docs/pages/VideoDecoder_High-Level_TestSpec.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestGetAndSetProperties) {
    // Fetch decoder IDs from the manager
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    int i = 0;

    for (const auto& id : decoderIds) {
        sp<IVideoDecoder> decoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(id, decoder);
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null for ID " << id.value;

        Capabilities capabilities;
        VIDEODECODER_GETCAPABILITIES(decoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            Codec _codec = _codecCaps.codec;

            sp<IVideoDecoderController> controller;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(decoder, _codec, capabilities.supportsSecure,
                                      controllerListener, controller, false);

            // Set and verify read-write properties
            for (int prop = static_cast<int>(::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE); prop <= static_cast<int>(::com::rdk::hal::videodecoder::Property::SHA1_CALC); ++prop) {
                Property property = static_cast<Property>(prop);
                PropertyValue setValue;
                setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);
                VIDEODECODER_SETPROPERTY_SUCCESS(controller,property,setValue);

                std::optional<PropertyValue> getValue;
                VIDEODECODER_GETPROPERTY(decoder,property,getValue);
                UT_ASSERT_EQUAL(getValue->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);
            }
            // Close the decoder
            VIDEODECODER_CLOSE_SUCCESS(decoder, controller, true);
        }
        i++;
    }
}

/**
* @brief Test the ability to read and set properties on multi instances of video decoder
*
* This test verifies the functionality of retrieving and setting properties on video decoders. It ensures that the decoder IDs can be retrieved, decoders can be opened and closed, and properties can be set and retrieved correctly.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 002@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_High-Level_TestSpec.md](../../docs/pages/VideoDecoder_High-Level_TestSpec.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestGetAndSetProperties_multipleInstance) {
    std::vector<sp<IVideoDecoder>>            decoders;
    std::vector<sp<IVideoDecoderController>>  controllers;
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);
    int index = 0;

    for (const auto& id : decoderIds) {
        sp<IVideoDecoder> decoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(id, decoder);
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null for ID " << id.value;

        Capabilities capabilities;
        VIDEODECODER_GETCAPABILITIES(decoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            Codec _codec = _codecCaps.codec;

            sp<IVideoDecoderController> controller;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();
            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(decoder, _codec, capabilities.supportsSecure,
                                      controllerListener, controller, false);
            decoders.push_back(decoder);
            controllers.push_back(controller);
            break;
        }
        index++;
    }

    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<IVideoDecoderController> controller = controllers[i];

        for (int prop = static_cast<int>(::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE); prop <= static_cast<int>(::com::rdk::hal::videodecoder::Property::SHA1_CALC); ++prop) {
            Property property = static_cast<Property>(prop);
            PropertyValue setValue;
            setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);

            VIDEODECODER_SETPROPERTY_SUCCESS(controller,property,setValue);

            std::optional<PropertyValue> getValue;
            VIDEODECODER_GETPROPERTY(decoder,property,getValue);
            UT_ASSERT_EQUAL(getValue->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);
        }
    }
    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<IVideoDecoderController> controller = controllers[i];
        VIDEODECODER_CLOSE_SUCCESS(decoder, controller, true);
    }
}


/**
* @brief Test the state transitions of video decoders
*
* This test verifies the ability to open, transition states, and close video decoders using the provided codec and secure mode settings. It ensures that the decoder transitions through the expected states and returns the correct status.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 003@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_High-Level_TestSpec.md](../../docs/pages/VideoDecoder_High-Level_TestSpec.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestDecoderStateTransitions) {
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    for (const auto& id : decoderIds) {
        sp<IVideoDecoder> decoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(id, decoder);
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null for ID " << id.value;

        Capabilities capabilities;
        VIDEODECODER_GETCAPABILITIES(decoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            Codec _codec = _codecCaps.codec;

            sp<IVideoDecoderController> controller;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(decoder, _codec, capabilities.supportsSecure,
                                      controllerListener, controller, true);
            VIDEODECODER_CHECK_STATE(decoder, State::READY);

            // Start to STARTED
            VIDEODECODER_START_SUCCESS(decoder, controller, true);
            VIDEODECODER_CHECK_STATE(decoder, State::STARTED);
            VIDEODECODER_FLUSH_SUCCESS(controller,true);
            VIDEODECODER_CHECK_STATE(decoder, State::READY);
            VIDEODECODER_START_SUCCESS(decoder, controller, true);
            VIDEODECODER_CHECK_STATE(decoder, State::STARTED);
            VIDEODECODER_FLUSH_SUCCESS(controller,false);
            VIDEODECODER_CHECK_STATE(decoder, State::STARTED);
            // Cleanup
            VIDEODECODER_STOP_SUCCESS(decoder, controller, true);
            VIDEODECODER_CHECK_STATE(decoder, State::READY);
            VIDEODECODER_CLOSE_SUCCESS(decoder, controller, true);
            VIDEODECODER_CHECK_STATE(decoder, State::CLOSED);
        }
    }
}


/**
* @brief Test the state transitions of multiple instances of video decoders
*
* This test verifies the ability to open, transition states, and close video decoders using the provided codec and secure mode settings. It ensures that the decoder transitions through the expected states and returns the correct status.
*
* **Test Group ID:** 02@n
* **Test Case ID:** 004@n
*
* **Test Procedure:**
* Refer to UT specification documentation [VideoDecoder_High-Level_TestSpec.md](../../docs/pages/VideoDecoder_High-Level_TestSpec.md)
*/
UT_ADD_TEST(VTS_L2_VideoDecoder, TestDecoderStateTransitions_multipleInstance) {
    std::vector<sp<IVideoDecoder>>            decoders;
    std::vector<sp<IVideoDecoderController>>  controllers;
    std::vector<IVideoDecoder::Id> decoderIds;
    VIDEODECODER_MANAGER_GETVIDEODECODERIDS(decoderIds);

    for (const auto& id : decoderIds) {
        sp<IVideoDecoder> decoder;
        VIDEODECODER_MANAGER_GETVIDEODECODER_SUCCESS(id, decoder);
        UT_ASSERT_TRUE(decoder != nullptr) << "Decoder pointer is null for ID " << id.value;

        Capabilities capabilities;
        VIDEODECODER_GETCAPABILITIES(decoder, &capabilities);

        for (const auto& _codecCaps : capabilities.supportedCodecs) {
            Codec _codec = _codecCaps.codec;

            sp<IVideoDecoderController> controller;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            // Open to READY
            VIDEODECODER_OPEN_SUCCESS(decoder, _codec, capabilities.supportsSecure,
                                      controllerListener, controller, true);
            decoders.push_back(decoder);
            controllers.push_back(controller);
            break;
        }
    }

    ::com::rdk::hal::State currentState;
    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<IVideoDecoderController> controller = controllers[i];

        // Start to STARTED
        VIDEODECODER_START_SUCCESS(decoder, controller, true);
        VIDEODECODER_CHECK_STATE(decoder, State::STARTED);
        VIDEODECODER_FLUSH_SUCCESS(controller,true);
        VIDEODECODER_CHECK_STATE(decoder, State::READY);
        VIDEODECODER_START_SUCCESS(decoder, controller, true);
        VIDEODECODER_CHECK_STATE(decoder, State::STARTED);
        VIDEODECODER_FLUSH_SUCCESS(controller,false);
        VIDEODECODER_CHECK_STATE(decoder, State::STARTED);
        // Cleanup
        VIDEODECODER_STOP_SUCCESS(decoder, controller, true);
        VIDEODECODER_CHECK_STATE(decoder, State::READY);
        VIDEODECODER_CLOSE_SUCCESS(decoder, controller, true);
        VIDEODECODER_CHECK_STATE(decoder, State::CLOSED);
    }

}


/** @} */ // End of Video_Decoder_Halif_Test_L2
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK