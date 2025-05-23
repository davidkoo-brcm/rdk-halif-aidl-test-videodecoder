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
    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];

        std::string supported_secure_path_key =  "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
        bool secure = UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

        int codecCount = 0;
        std::string supported_codec_count_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs";
        codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supported_codec_count_key.c_str());

        for(int j = 0; j < codecCount; ++j){
            char codecName[UT_KVP_MAX_ELEMENT_SIZE];
            std::string codecpath_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);
            Codec codec = Codec::MPEG;
            if (codecName == "H264") codec = Codec::H264;
            else if (codecName == "H265") codec = Codec::H265;
            else if (codecName == "VP9") codec = Codec::VP9;
            else if (codecName == "AV1") codec = Codec::AV1;

            sp<IVideoDecoderController> controller;
            sp<VideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            UT_LOG_INFO("Calling open(IN:codecName[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[])", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener.get());
            Status status = decoder->open(codec, secure, controllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(controller != nullptr);

            // Set and verify read-write properties
            for (int prop = static_cast<int>(::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE); prop <= static_cast<int>(::com::rdk::hal::videodecoder::Property::SHA1_CALC); ++prop) {
                Property property = static_cast<Property>(prop);
                PropertyValue setValue;
                setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);

                bool setResult = false;
                UT_LOG_INFO("Calling setProperty(IN:property:[%s], IN:setValue:[1]), OUT:setResult[]", com::rdk::hal::videodecoder::toString(property).c_str());
                Status setStatus = controller->setProperty(property, setValue, &setResult);
                UT_LOG_INFO("Result setProperty(IN:property:[%s], IN:setValue:[1]), setResult[%s], setStatus:[isOK:%s, exception:%s]", com::rdk::hal::videodecoder::toString(property).c_str(), UT_Control_GetMapString(bool_mapTable, setStatus.isOk()), UT_Control_GetMapString(bool_mapTable, setResult), UT_Control_GetMapString(statusException_mapTable, setStatus.exceptionCode()));
                UT_ASSERT_TRUE(setStatus.isOk());
                UT_ASSERT_TRUE(setResult);

                std::optional<PropertyValue> getValue;
                UT_LOG_INFO("Calling getProperty(IN:property:[%s], OUT:getValue[])", com::rdk::hal::videodecoder::toString(property).c_str());
                Status getStatus = decoder->getProperty(property, &getValue);
                UT_LOG_INFO("Result getProperty(IN:property:[%s], OUT:getValue:[%d]), getStatus:[isOK:%s, exception:%s]", com::rdk::hal::videodecoder::toString(property).c_str(), getValue->value->getTag(), UT_Control_GetMapString(bool_mapTable, getStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, getStatus.exceptionCode()));
                UT_ASSERT_TRUE(getStatus.isOk());
                UT_ASSERT_TRUE(getValue.has_value());
                UT_ASSERT_EQUAL(getValue->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);
            }
            // Close the decoder
            bool closeResult = false;
            UT_LOG_INFO("Calling close(IN:controller)");
            Status closestatus = decoder->close(controller, &closeResult);
            UT_LOG_INFO("Result close(IN:controller[0x%X], OUT:closeResult[%d]), closestatus:[isOK:%s, exception:%s]", controller.get(), UT_Control_GetMapString(bool_mapTable, closeResult), UT_Control_GetMapString(bool_mapTable, closestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, closestatus.exceptionCode()));
            UT_ASSERT_TRUE(closestatus.isOk());
            UT_ASSERT_TRUE(closeResult);
        }
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
    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<IVideoDecoderController> controller = controllers[i];

        for (int prop = static_cast<int>(::com::rdk::hal::videodecoder::Property::OPERATIONAL_MODE); prop <= static_cast<int>(::com::rdk::hal::videodecoder::Property::SHA1_CALC); ++prop) {
            Property property = static_cast<Property>(prop);
            PropertyValue setValue;
            setValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(1);

            bool setResult = false;
            UT_LOG_INFO("Calling setProperty(IN:property:[%s], IN:setValue:[1]), OUT:setResult[]", com::rdk::hal::videodecoder::toString(property).c_str());
            Status setStatus = controller->setProperty(property, setValue, &setResult);
            UT_LOG_INFO("Result setProperty(IN:property:[%s], IN:propertyValue:[1]), setResult[%s], setStatus:[isOK:%s, exception:%s]", com::rdk::hal::videodecoder::toString(property).c_str(), UT_Control_GetMapString(bool_mapTable, setResult), UT_Control_GetMapString(bool_mapTable, setStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, setStatus.exceptionCode()));
            UT_ASSERT_TRUE(setStatus.isOk());
            UT_ASSERT_TRUE(setResult);

            std::optional<PropertyValue> getValue;
            UT_LOG_INFO("Calling getProperty(IN:property:[%s], OUT:getValue[])", com::rdk::hal::videodecoder::toString(property).c_str());
            Status getStatus = decoder->getProperty(property, &getValue);
            UT_LOG_INFO("Result getProperty(IN:property:[%s], OUT:getValue:[%d]), getStatus:[isOK:%s, exception:%s]", com::rdk::hal::videodecoder::toString(property).c_str(), getValue->value->getTag(), UT_Control_GetMapString(bool_mapTable, getStatus.isOk()), UT_Control_GetMapString(statusException_mapTable, getStatus.exceptionCode()));
            UT_ASSERT_TRUE(getStatus.isOk());
            UT_ASSERT_TRUE(getValue.has_value());
            UT_ASSERT_EQUAL(getValue->value->getTag(), ::com::rdk::hal::PropertyValue::Value::Tag::intValue);
        }
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
    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<VideoDecoderEventListener> listener = new VideoDecoderEventListener();

        std::string supported_secure_path_key =  "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
        bool secure = UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

        int codecCount = 0;
        std::string supported_codec_count_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs";
        codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supported_codec_count_key.c_str());

        for(int j = 0; j < codecCount; ++j){
            char codecName[UT_KVP_MAX_ELEMENT_SIZE];
            std::string codecpath_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);

            Codec codec = Codec::MPEG;
            if (codecName == "H264") codec = Codec::H264;
            else if (codecName == "H265") codec = Codec::H265;
            else if (codecName == "VP9") codec = Codec::VP9;
            else if (codecName == "AV1") codec = Codec::AV1;

            sp<IVideoDecoderController> controller;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[])", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener.get());
            Status status = decoder->open(codec, secure, controllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(controller != nullptr);



            ::com::rdk::hal::State currentState;
            UT_LOG_INFO("Calling getState(OUT:currentState[])");
            Status statestatus = decoder->getState(&currentState);
            UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, State::READY);

            // Start decoder
            UT_LOG_INFO("Calling start()");
            status = controller->start();
            UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());



            UT_LOG_INFO("Calling getState(OUT:currentState[])");
            statestatus = decoder->getState(&currentState);
            UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, State::STARTED);

            // Flush decoder
            UT_LOG_INFO("Calling flush(IN:reset[%d])", false);
            status = controller->flush(false);
            UT_LOG_INFO("Result flush(), status:[isOK:%s, exception:%s]",UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());



            UT_LOG_INFO("Calling getState(OUT:currentState[])");
            statestatus = decoder->getState(&currentState);
            UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, State::STARTED);

            // Stop decoder
            UT_LOG_INFO("Calling stop()");
            status = controller->stop();
            UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());



            UT_LOG_INFO("Calling getState(OUT:currentState[])");
            statestatus = decoder->getState(&currentState);
            UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, State::READY);

            // Close the decoder
            bool result = false;
            UT_LOG_INFO("Calling close(IN:controller:[0x%X], OUT:result)", controller.get());
            status = decoder->close(controller, &result);
            UT_LOG_INFO("Result close(IN:controller:[0x%X], OUT:result:[%d]), status:[isOK:%s, exception:%s]", controller.get(), result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(result);



            UT_LOG_INFO("Calling getState(OUT:currentState[])");
            statestatus = decoder->getState(&currentState);
            UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, State::CLOSED);
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
    ::com::rdk::hal::State currentState;

    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<IVideoDecoderEventListener> listener = new VideoDecoderEventListener();

        std::string supported_secure_path_key =  "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
        bool secure = UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

        int codecCount = 0;
        std::string supported_codec_count_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs";
        codecCount = UT_KVP_PROFILE_GET_LIST_COUNT(supported_codec_count_key.c_str());

        for(int j = 0; j < codecCount; ++j){
            char codecName[UT_KVP_MAX_ELEMENT_SIZE];
            std::string codecpath_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);

            Codec codec = Codec::MPEG;
            if (codecName == "H264") codec = Codec::H264;
            else if (codecName == "H265") codec = Codec::H265;
            else if (codecName == "VP9") codec = Codec::VP9;
            else if (codecName == "AV1") codec = Codec::AV1;

            sp<IVideoDecoderController> controller;
            sp<IVideoDecoderControllerListener> controllerListener = new VideoDecoderControllerListener();

            UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[])", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener.get());
            Status status = decoder->open(codec, secure, controllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(controller != nullptr);
            if (controller != nullptr){
                controllers.push_back(controller);
            }



            UT_LOG_INFO("Calling getState(OUT:currentState[])");
            Status statestatus = decoder->getState(&currentState);
            UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, State::READY);
        }
    }

    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();
        sp<IVideoDecoderController> controller = controllers[i];

        // Start decoder
        UT_LOG_INFO("Calling start()");
        Status status = controller->start();
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());



        UT_LOG_INFO("Calling getState(OUT:currentState[])");
        Status statestatus = decoder->getState(&currentState);
        UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(currentState, State::STARTED);

        // Flush decoder
        UT_LOG_INFO("Calling flush(IN:reset[%d])", false);
        status = controller->flush(false);
        UT_LOG_INFO("Result flush(), status:[isOK:%s, exception:%s]",UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());



        UT_LOG_INFO("Calling getState(OUT:currentState[])");
        statestatus = decoder->getState(&currentState);
        UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(currentState, State::STARTED);

        // Stop decoder
        UT_LOG_INFO("Calling stop()");
        status = controller->stop();
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());



        UT_LOG_INFO("Calling getState(OUT:currentState[])");
        statestatus = decoder->getState(&currentState);
        UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(currentState, State::READY);
    }

    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();
        sp<IVideoDecoderController> controller = controllers[i];

        // Close the decoder
        bool result = false;
        UT_LOG_INFO("Calling close(IN:controller:[0x%X], OUT:result)", controller.get());
        Status status = decoder->close(controller, &result);
        UT_LOG_INFO("Result close(IN:controller:[0x%X], OUT:result:[%d]), status:[isOK:%s, exception:%s]", controller.get(), result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(result);



        UT_LOG_INFO("Calling getState(OUT:currentState[])");
        Status statestatus = decoder->getState(&currentState);
        UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(currentState, State::CLOSED);
    }
}


/** @} */ // End of Video_Decoder_Halif_Test_L2
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK