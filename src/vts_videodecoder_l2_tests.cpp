
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
 
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <optional>
#include <ut.h>
#include <ut_kvp_profile.h>
#include <ut_log.h>
#include <ut_control_plane.h>
#include <deque>

#include "com/rdk/hal/videodecoder/IVideoDecoderManager.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderControllerListener.h"
#include "com/rdk/hal/videodecoder/BnVideoDecoderEventListener.h"

using namespace com::rdk::hal;
using namespace com::rdk::hal::videodecoder;

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;

static constexpr int MAX_TRANSITORY_STATE_WAIT_MS = 10;
static constexpr int MAX_FINAL_STATE_WAIT_MS = 500;

static std::vector<IVideoDecoder::Id>            decoderIds;
static std::vector<sp<IVideoDecoder>>            decoders;
static std::vector<sp<IVideoDecoderController>>  controllers;

/* Status::Exception */
const static ut_control_keyStringMapping_t statusException_mapTable[] =
{
    {"EX_NONE",                  (int32_t) Status::EX_NONE},
    {"EX_SECURITY",              (int32_t) Status::EX_SECURITY},
    {"EX_BAD_PARCELABLE",        (int32_t) Status::EX_BAD_PARCELABLE},
    {"EX_ILLEGAL_ARGUMENT",      (int32_t) Status::EX_ILLEGAL_ARGUMENT},
    {"EX_NULL_POINTER",          (int32_t) Status::EX_NULL_POINTER},
    {"EX_ILLEGAL_STATE",         (int32_t) Status::EX_ILLEGAL_STATE},
    {"EX_NETWORK_MAIN_THREAD",   (int32_t) Status::EX_NETWORK_MAIN_THREAD},
    {"EX_UNSUPPORTED_OPERATION", (int32_t) Status::EX_UNSUPPORTED_OPERATION},
    {"EX_SERVICE_SPECIFIC",      (int32_t) Status::EX_SERVICE_SPECIFIC},
    {"EX_PARCELABLE",            (int32_t) Status::EX_PARCELABLE},
    {"EX_HAS_REPLY_HEADER",      (int32_t) Status::EX_HAS_REPLY_HEADER},
    {"EX_TRANSACTION_FAILED",    (int32_t) Status::EX_TRANSACTION_FAILED},
    {NULL, -1}
};

/* bool */
const static ut_control_keyStringMapping_t bool_mapTable[] =
{
    {"false", (int32_t) false},
    {"true",  (int32_t) true},
    {NULL, -1}
};


sp<IVideoDecoderManager> service;
	
int getService() {
    service = android::interface_cast<IVideoDecoderManager>(
            defaultServiceManager()->getService(String16(IVideoDecoderManager::serviceName().c_str())));
    if (service == nullptr) {
        printf("Service not available\n");
        return -1;
    }
    return 0;
}
 
class VideoDecoderManagerBaseTest : public virtual UTCore {
protected:
    void SetUp() override {
		UTCore::SetUp();

        UT_ASSERT_EQUAL(getService(), 0) << "Failed to get IVideoDecoderManager service";
        UT_ASSERT_TRUE(service != nullptr);
    }
 
    void TearDown() override {
		UTCore::TearDown();

        service.clear();
    }
};

class MyVideoDecoderEventListener : public BnVideoDecoderEventListener {
public:
    std::mutex mtx;
    std::condition_variable cv;
    std::deque<std::pair<State, State>> stateChanges;
    
    Status onStateChanged(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState) override {
        std::lock_guard<std::mutex> lock(mtx);
        stateChanges.emplace_back(oldState, newState);
        cv.notify_all();
        return Status::ok();
    }

    Status onDecodeError(::com::rdk::hal::videodecoder::ErrorCode error, int32_t vendorerrorCode) override {
		return ::android::binder::Status::ok();
    }

    bool waitForState(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState, int timeoutMs = 500) {
        std::unique_lock<std::mutex> lock(mtx);
        return cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] {
            for (const auto& p : stateChanges) {
                if (p.first == oldState && p.second == newState) return true;
            }
            return false;
        });
    }
};

class VideoDecoderBaseTest : public VideoDecoderManagerBaseTest {
public:
    std::vector<sp<MyVideoDecoderEventListener>> decoderEventListeners;

    void SetUp() override {
		UTCore::SetUp();

        VideoDecoderManagerBaseTest::SetUp();
		
		// Get list of Video Decoder IDs
        std::vector<IVideoDecoder::Id> tmpIds;
        UT_LOG_INFO("Calling getVideoDecoderIds(OUT:tmpIds:[])");
        Status status = service->getVideoDecoderIds(&tmpIds);
        UT_LOG_INFO("Result getVideoDecoderIds(OUT:tmpIds:[size: %d]), status:[isOK:%s, exception:%s]", tmpIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_FALSE(tmpIds.empty()) << "No VideoDecoder IDs found.";
        decoderIds = tmpIds;

        for (const auto& id : decoderIds) {
            sp<IVideoDecoder> decoder;
            UT_LOG_INFO("Calling getVideoDecoder(IN:id:[%d], OUT:decoder[])", id.value);
            Status status = service->getVideoDecoder(id, &decoder);
            UT_LOG_INFO("Result getVideoDecoder(IN:id:[%d], OUT:decoder[0x%X]), status:[isOK:%s, exception:%s]", id.value, decoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(decoder != nullptr);
             
            sp<MyVideoDecoderEventListener> listener = new MyVideoDecoderEventListener();
            
            if (decoder != nullptr) {
                bool registered = false;
                UT_LOG_INFO("Calling registerEventListener(IN:listener[0x%X], OUT:registered[])", listener.get());
                Status status = decoder->registerEventListener(listener, &registered);
                UT_LOG_INFO("Result registerEventListener(IN:listener[0x%X], OUT:registered[%s]), status:[isOK:%s, exception:%s]", listener.get(), UT_Control_GetMapString(bool_mapTable, registered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
                UT_ASSERT_TRUE(status.isOk());
                UT_ASSERT_TRUE(registered);

                decoders.push_back(decoder);
                decoderEventListeners.push_back(listener);
            }
        }
    }

    void TearDown() override {
		UTCore::TearDown();

        for (size_t i = 0; i < decoders.size(); ++i) {
            bool unregistered = false;
            UT_LOG_INFO("Calling unregisterEventListener(IN:decoderEventListeners[0x%X], OUT:unregistered[])", decoderEventListeners[i].get());
            Status status = decoders[i]->unregisterEventListener(decoderEventListeners[i], &unregistered);
            UT_LOG_INFO("Result unregisterEventListener(IN:decoderEventListeners[0x%X], OUT:unregistered[%s]), status:[isOK:%s, exception:%s]", decoderEventListeners[i].get(), UT_Control_GetMapString(bool_mapTable, unregistered), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(unregistered);
        }

        decoders.clear();
		decoderIds.clear();
        decoderEventListeners.clear();
        VideoDecoderManagerBaseTest::TearDown();
    }
};


class MyVideoDecoderControllerListener : public BnVideoDecoderControllerListener {
public:

    Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::videodecoder::FrameMetadata>& metadata) override
    {
        return ::android::binder::Status::ok();
    }
    Status onUserDataOutput(int64_t nsPresentationTime, const ::std::vector<uint8_t>& userData) override
    {
        return ::android::binder::Status::ok();
    }
};


class VideoDecoderControllerBaseTest : public VideoDecoderBaseTest {
public:
    std::vector<sp<IVideoDecoder>> openedDecoders;
    std::vector<sp<MyVideoDecoderControllerListener>> controllerListeners;

    void SetUp() override {
		UTCore::SetUp();

        VideoDecoderBaseTest::SetUp();

        for (size_t i = 0; i < decoders.size(); ++i) {
            sp<IVideoDecoderController> controller;
            sp<MyVideoDecoderControllerListener> controllerListener = new MyVideoDecoderControllerListener();
 
            std::string supported_secure_path_key =  "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
            bool secure = UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

            char codecName[UT_KVP_MAX_ELEMENT_SIZE];
			std::string codecpath_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(0);
			UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);
            Codec codec = Codec::MPEG;
            if (codecName == "H264") codec = Codec::H264;
            else if (codecName == "H265") codec = Codec::H265;
            else if (codecName == "VP9") codec = Codec::VP9;
            else if (codecName == "AV1") codec = Codec::AV1;
            
            UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[])", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener.get());
            Status status = decoders[i]->open(codec, secure, controllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codecName, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(controller != nullptr);
            
            controllers.push_back(controller);
            openedDecoders.push_back(decoders[i]);
            controllerListeners.push_back(controllerListener);
        }
    }

    void TearDown() override {
		UTCore::TearDown();

        for (size_t i = 0; i < controllers.size(); ++i) {
            bool result = false;
            UT_LOG_INFO("Calling close(IN:controllers[0x%X], OUT:result[])", controllers[i].get());
            Status status = openedDecoders[i]->close(controllers[i], &result);
            UT_LOG_INFO("Result close(IN:controllers[0x%X], OUT:result[%s]), status:[isOK:%s, exception:%s]", controllers[i].get(), UT_Control_GetMapString(bool_mapTable, result), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            EXPECT_TRUE(result);
        }

        controllers.clear();
        openedDecoders.clear();
        controllerListeners.clear();
        VideoDecoderBaseTest::TearDown();
    }
};


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
UT_ADD_TEST(VideoDecoderBaseTest, TestGetAndSetProperties) {
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
            sp<MyVideoDecoderControllerListener> controllerListener = new MyVideoDecoderControllerListener();

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
UT_ADD_TEST(VideoDecoderControllerBaseTest, TestGetAndSetProperties_multipleInstance) {
    for (size_t i = 0; i < openedDecoders.size(); ++i) {
        sp<IVideoDecoder> decoder = openedDecoders[i];
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
UT_ADD_TEST(VideoDecoderBaseTest, TestDecoderStateTransitions) {
    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<MyVideoDecoderEventListener> listener = decoderEventListeners[i];
 
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
            sp<MyVideoDecoderControllerListener> controllerListener = new MyVideoDecoderControllerListener();

            UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[])", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener.get());
            Status status = decoder->open(codec, secure, controllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(controller != nullptr);

            // Verify state transition to OPENING
            UT_ASSERT_TRUE(listener->waitForState(State::CLOSED, State::OPENING, MAX_TRANSITORY_STATE_WAIT_MS));

            // Verify state transition to READY
            UT_ASSERT_TRUE(listener->waitForState(State::OPENING, State::READY, MAX_FINAL_STATE_WAIT_MS));
            
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

            // Verify state transition to STARTING
            UT_ASSERT_TRUE(listener->waitForState(State::READY, State::STARTING, MAX_TRANSITORY_STATE_WAIT_MS));

            // Verify state transition to STARTED
            UT_ASSERT_TRUE(listener->waitForState(State::STARTING, State::STARTED, MAX_FINAL_STATE_WAIT_MS));
            
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

            // Verify state transition to FLUSHING
            UT_ASSERT_TRUE(listener->waitForState(State::STARTED, State::FLUSHING, MAX_TRANSITORY_STATE_WAIT_MS));

            // Verify state transition to STARTED
            UT_ASSERT_TRUE(listener->waitForState(State::FLUSHING, State::STARTED, MAX_FINAL_STATE_WAIT_MS));
            
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

            // Verify state transition to STOPPING
            UT_ASSERT_TRUE(listener->waitForState(State::STARTED, State::STOPPING, MAX_TRANSITORY_STATE_WAIT_MS));

            // Verify state transition to READY
            UT_ASSERT_TRUE(listener->waitForState(State::STOPPING, State::READY, MAX_FINAL_STATE_WAIT_MS));
            
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

            // Verify state transition to CLOSING
            UT_ASSERT_TRUE(listener->waitForState(State::READY, State::CLOSING, MAX_TRANSITORY_STATE_WAIT_MS));

            // Verify state transition to CLOSED
            UT_ASSERT_TRUE(listener->waitForState(State::CLOSING, State::CLOSED, MAX_FINAL_STATE_WAIT_MS));
           
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
UT_ADD_TEST(VideoDecoderBaseTest, TestDecoderStateTransitions_multipleInstance) {
    ::com::rdk::hal::State currentState;

    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<MyVideoDecoderEventListener> listener = decoderEventListeners[i];
    
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
            sp<MyVideoDecoderControllerListener> controllerListener = new MyVideoDecoderControllerListener();

            UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[])", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener.get());
            Status status = decoder->open(codec, secure, controllerListener, &controller);
            UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[0x%X], OUT:controller[0x%X]), status:[isOK:%s, exception:%s]", codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, controller.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_TRUE(controller != nullptr);
            if (controller != nullptr){
                controllers.push_back(controller);
            }

            // Verify state transition to OPENING
            UT_ASSERT_TRUE(listener->waitForState(State::CLOSED, State::OPENING, MAX_TRANSITORY_STATE_WAIT_MS));

            // Verify state transition to READY
            UT_ASSERT_TRUE(listener->waitForState(State::OPENING, State::READY, MAX_FINAL_STATE_WAIT_MS));
            
            UT_LOG_INFO("Calling getState(OUT:currentState[])");
            Status statestatus = decoder->getState(&currentState);
            UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
            UT_ASSERT_TRUE(status.isOk());
            UT_ASSERT_EQUAL(currentState, State::READY);
        }
    }

    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<MyVideoDecoderEventListener> listener = decoderEventListeners[i];
        sp<IVideoDecoderController> controller = controllers[i];

        // Start decoder
        UT_LOG_INFO("Calling start()");
        Status status = controller->start();
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());

        // Verify state transition to STARTING
        UT_ASSERT_TRUE(listener->waitForState(State::READY, State::STARTING, MAX_TRANSITORY_STATE_WAIT_MS));

        // Verify state transition to STARTED
        UT_ASSERT_TRUE(listener->waitForState(State::STARTING, State::STARTED, MAX_FINAL_STATE_WAIT_MS));
        
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

        // Verify state transition to FLUSHING
        UT_ASSERT_TRUE(listener->waitForState(State::STARTED, State::FLUSHING, MAX_TRANSITORY_STATE_WAIT_MS));

        // Verify state transition to STARTED
        UT_ASSERT_TRUE(listener->waitForState(State::FLUSHING, State::STARTED, MAX_FINAL_STATE_WAIT_MS));
        
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

        // Verify state transition to STOPPING
        UT_ASSERT_TRUE(listener->waitForState(State::STARTED, State::STOPPING, MAX_TRANSITORY_STATE_WAIT_MS));

        // Verify state transition to READY
        UT_ASSERT_TRUE(listener->waitForState(State::STOPPING, State::READY, MAX_FINAL_STATE_WAIT_MS));
        
        UT_LOG_INFO("Calling getState(OUT:currentState[])");
        statestatus = decoder->getState(&currentState);
        UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(currentState, State::READY);
    }

    for (size_t i = 0; i < decoderIds.size(); ++i) {
        sp<IVideoDecoder> decoder = decoders[i];
        sp<MyVideoDecoderEventListener> listener = decoderEventListeners[i];
        sp<IVideoDecoderController> controller = controllers[i];

        // Close the decoder
        bool result = false;
        UT_LOG_INFO("Calling close(IN:controller:[0x%X], OUT:result)", controller.get());
        Status status = decoder->close(controller, &result);
        UT_LOG_INFO("Result close(IN:controller:[0x%X], OUT:result:[%d]), status:[isOK:%s, exception:%s]", controller.get(), result, UT_Control_GetMapString(bool_mapTable, status.isOk()), UT_Control_GetMapString(statusException_mapTable, status.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_TRUE(result);

        // Verify state transition to CLOSING
        UT_ASSERT_TRUE(listener->waitForState(State::READY, State::CLOSING, MAX_TRANSITORY_STATE_WAIT_MS));

        // Verify state transition to CLOSED
        UT_ASSERT_TRUE(listener->waitForState(State::CLOSING, State::CLOSED, MAX_FINAL_STATE_WAIT_MS));
        
        UT_LOG_INFO("Calling getState(OUT:currentState[])");
        Status statestatus = decoder->getState(&currentState);
        UT_LOG_INFO("Result getState(OUT:currentState:[%s]), statestatus:[isOK:%s, exception:%s]", currentState, UT_Control_GetMapString(bool_mapTable, statestatus.isOk()), UT_Control_GetMapString(statusException_mapTable, statestatus.exceptionCode()));
        UT_ASSERT_TRUE(status.isOk());
        UT_ASSERT_EQUAL(currentState, State::CLOSED);
    }
}

int main(int argc, char **argv)
{
    UT_status_t result;
    UT_init(argc, argv);
    result = UT_run_tests();
    return result;
}
