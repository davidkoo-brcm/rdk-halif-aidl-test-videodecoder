
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
 #include <optional>
 #include <algorithm>
 #include <unordered_set>

 #include "com/rdk/hal/videodecoder/IVideoDecoderManager.h"
 #include "com/rdk/hal/videodecoder/BnVideoDecoderControllerListener.h"
 #include "com/rdk/hal/videodecoder/BnVideoDecoderEventListener.h"
 #include "com/rdk/hal/videodecoder/DynamicRange.h"
 #include "com/rdk/hal/videodecoder/Codec.h"
 #include "com/rdk/hal/videodecoder/CodecCapabilities.h"
 #include "com/rdk/hal/videodecoder/OperationalMode.h"

 using namespace com::rdk::hal;
 using namespace com::rdk::hal::videodecoder;

 using android::sp;
 using android::String16;
 using android::defaultServiceManager;
 using android::binder::Status;

 static constexpr int MAX_TRANSITORY_STATE_WAIT_MS = 10;
 static constexpr int MAX_FINAL_STATE_WAIT_MS = 500;

// Global test data (consider moving to a singleton or test fixture if shared state is needed)

extern sp<IVideoDecoderManager> service;
extern std::vector<IVideoDecoder::Id>            decoderIds;
extern std::vector<sp<IVideoDecoder>>            decoders;
extern std::vector<sp<IVideoDecoderController>>  controllers;

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


class VideoDecoderControllerListener : public BnVideoDecoderControllerListener {
public:

    VideoDecoderControllerListener() {};
    virtual ~VideoDecoderControllerListener() = default;

    Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::std::optional<::com::rdk::hal::videodecoder::FrameMetadata>& metadata) override
    {
        return ::android::binder::Status::ok();
    }
    Status onUserDataOutput(int64_t nsPresentationTime, const ::std::vector<uint8_t>& userData) override
    {
        return ::android::binder::Status::ok();
    }
};

class VideoDecoderEventListener : public BnVideoDecoderEventListener {
public:
    VideoDecoderEventListener() {};
    virtual ~VideoDecoderEventListener() = default;

    Status onDecodeError(::com::rdk::hal::videodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override {
        return ::android::binder::Status::ok();
    }
    Status onStateChanged(::com::rdk::hal::State oldState, ::com::rdk::hal::State newState) override {
        return ::android::binder::Status::ok();
    }
};


class VideoDecoderManagerBaseTest : public virtual UTCore {
protected:
    void SetUp() override {
    }

    void TearDown() override {
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
protected:
    std::vector<sp<IVideoDecoderController>> controllers;
    std::vector<sp<IVideoDecoder>> openedDecoders;

    void SetUp() override {
		UTCore::SetUp();

        VideoDecoderBaseTest::SetUp();

        for (size_t i = 0; i < decoders.size(); ++i) {
            sp<IVideoDecoderController> controller;
            sp<IVideoDecoderControllerListener> listener = new VideoDecoderControllerListener();

            std::string supported_secure_path_key =  "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportsSecure";
            bool secure = UT_KVP_PROFILE_GET_BOOL(supported_secure_path_key.c_str());

			char codecName[UT_KVP_MAX_ELEMENT_SIZE];
			std::string codecpath_key = "IVideoDecoderManager.IVideoDecoder." + std::to_string(i) + ".supportedCodecs." + std::to_string(0);
			UT_KVP_PROFILE_GET_STRING(codecpath_key.c_str(), codecName);

            Codec codec = Codec::MPEG; // Default value, replace with actual mapping from string to enum
            if (codecName == "H264") codec = Codec::H264;
            else if (codecName == "H265") codec = Codec::H265;
            else if (codecName == "VP9") codec = Codec::VP9;
            else if (codecName == "AV1") codec = Codec::AV1;


            android::binder::Status status = decoders[i]->open(codec, secure, listener, &controller);
            if (status.isOk() && controller != nullptr) {
                controllers.push_back(controller);
                openedDecoders.push_back(decoders[i]);
            }
        }

        UT_ASSERT_FALSE(controllers.empty()) << "Failed to open any VideoDecoderController.";
    }

    void TearDown() override {
		UTCore::TearDown();

        for (size_t i = 0; i < controllers.size(); ++i) {
            bool result = false;
            openedDecoders[i]->close(controllers[i], &result);
            UT_ASSERT_TRUE(result);
        }
        controllers.clear();
        openedDecoders.clear();
        VideoDecoderBaseTest::TearDown();
    }
};

#endif //__VTS_VIDEODECODER_H__