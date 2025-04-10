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
/** @brief
* VideoDecoder.cpp
*
* Implementation of VideoDecoder service
*/

#include "VideoDecoder.h"
#include <iostream>
#include <chrono>
#include <thread>


namespace com {
    namespace rdk {
        namespace hal {
            namespace videodecoder {

                /* Read only Properties */
                State g_videodecoder_state[NUM_VIDEO_DECODER] = {State::UNKNOWN, State::UNKNOWN};
                ::std::vector<::com::rdk::hal::videodecoder::Codec> supportedCodecs = {
                        com::rdk::hal::videodecoder::Codec::MPEG,
                        com::rdk::hal::videodecoder::Codec::H264,
                        com::rdk::hal::videodecoder::Codec::VP9,
                        com::rdk::hal::videodecoder::Codec::AV1,
                        com::rdk::hal::videodecoder::Codec::H265
                };
                IVideoDecoder::Id g_supported_id[NUM_VIDEO_DECODER];
                int32_t g_support_secure[NUM_VIDEO_DECODER];
                int32_t g_metrics_frame_decoded[NUM_VIDEO_DECODER];
                int32_t g_metrics_decode_errors[NUM_VIDEO_DECODER];
                int32_t g_metrics_frames_dropped[NUM_VIDEO_DECODER];
                int32_t g_input_queue_depth[NUM_VIDEO_DECODER];
                int32_t g_output_frame_pool_size[NUM_VIDEO_DECODER];

                /* Read Write Properties */
                int32_t g_low_latency[NUM_VIDEO_DECODER];
                AVSource g_av_source[NUM_VIDEO_DECODER];
                int32_t g_sha1_calc[NUM_VIDEO_DECODER];
                int32_t g_decode_error_policy[NUM_VIDEO_DECODER];
                int32_t g_operational_mode[NUM_VIDEO_DECODER];

                std::mutex state_mutex;
                std::condition_variable state_cv;

                VideoDecoderManager::VideoDecoderManager()
                {
                    for (int32_t i = 0;i<NUM_VIDEO_DECODER;i++) {
                        g_supported_id[i].value = i;
                        g_low_latency[i] = 0;
                        g_av_source[i] = AVSource::UNKNOWN;
                        g_support_secure[i] = 1;
                        g_metrics_frame_decoded[i] = 0;
                        g_metrics_decode_errors[i] = 0;
                        g_metrics_frames_dropped[i] = 0;
                        g_operational_mode[i] = 2;
                        g_input_queue_depth[i] = 96000;
                        g_decode_error_policy[i] = 0;
                        g_sha1_calc[i] = 0;
                        g_output_frame_pool_size[i] = 5;
                    }
                    printf("VideoDecoderManager service initialized\n");
                }

                VideoDecoderManager::~VideoDecoderManager()
                {
                    printf("VideoDecoderManager service destroyed\n");
                }
                android::sp<videodecoder::IVideoDecoderEventListener> _videoDecoderEventListener;

                android::binder::Status VideoDecoderManager::getVideoDecoderIds(  std::vector<IVideoDecoder::Id>* _aidl_return  )
                {
                    for (int32_t i = 0;i<NUM_VIDEO_DECODER;i++)
                    {
                        _aidl_return->push_back(g_supported_id[i]);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoderManager::getSupportedOperationalModes(  std::vector<OperationalMode>* _aidl_return  )
                {
                    _aidl_return->push_back(OperationalMode::TUNNELLED);
                    _aidl_return->push_back(OperationalMode::NON_TUNNELLED);
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoderManager::getVideoDecoder(  const IVideoDecoder::Id& decoderResourceId, android::sp<IVideoDecoder>* _aidl_return  )
                {
                    int32_t found = 0;
                    for(int32_t i = 0; i < NUM_VIDEO_DECODER; i++)
                    {
                        if(g_supported_id[i].value == decoderResourceId.value)
                        {
                            found = 1;
                            break;
                        }
                    }
                    if(!found)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }

                    android::sp<IVideoDecoder> videoDecoder = new VideoDecoder(decoderResourceId);
                    g_videodecoder_state[decoderResourceId.value] = State::CLOSED;
                    *_aidl_return = videoDecoder;
                    return android::binder::Status::ok();
                }

                VideoDecoder::VideoDecoder(IVideoDecoder::Id id)
                {
                    this->id = id;
                    this->event_listener = false;
                    printf("VideoDecoder service initialized\n");
                }

                VideoDecoder::~VideoDecoder()
                {
                    printf("VideoDecoder service destroyed\n");
                }

                android::binder::Status VideoDecoder::getCapabilities(  videodecoder::Capabilities* _aidl_return  )
                {
                    // Initialize the Capabilities object
                    Capabilities capabilities;

                    CodecCapabilities codec1;
                    codec1.codec = Codec::MPEG;
                    codec1.maxFrameRate = 60;
                    codec1.maxFrameWidth = 1920;
                    codec1.maxFrameHeight = 1080;

                    CodecCapabilities codec2;
                    codec1.codec = Codec::H264;
                    codec1.maxFrameRate = 60;
                    codec1.maxFrameWidth = 1920;
                    codec1.maxFrameHeight = 1080;

                    CodecCapabilities codec3;
                    codec1.codec = Codec::H265;
                    codec1.maxFrameRate = 60;
                    codec1.maxFrameWidth = 3840;
                    codec1.maxFrameHeight = 2160;

                    CodecCapabilities codec4;
                    codec1.codec = Codec::VP9;
                    codec1.maxFrameRate = 60;
                    codec1.maxFrameWidth = 3840;
                    codec1.maxFrameHeight = 2160;

                    CodecCapabilities codec5;
                    codec1.codec = Codec::AV1;
                    codec1.maxFrameRate = 60;
                    codec1.maxFrameWidth = 3840;
                    codec1.maxFrameHeight = 2160;

                    capabilities.supportedCodecs = {codec1, codec2, codec3, codec4, codec5};

                    capabilities.supportedDynamicRanges = {DynamicRange::SDR, DynamicRange::HDR10,
                                                           DynamicRange::DOLBY_VISION, DynamicRange::HLG};

                    capabilities.supportsSecure = true;
                    *_aidl_return = capabilities;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::getProperty(  videodecoder::Property property, std::optional<PropertyValue>* _aidl_return  )
                {
                    if (!_aidl_return)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }
                    com::rdk::hal::PropertyValue value;
                    switch (property) {
                        case videodecoder::Property::RESOURCE_ID:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->id.value);
                            break;
                        case videodecoder::Property::INPUT_QUEUE_DEPTH:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::byteValue>(g_input_queue_depth[this->id.value]);
                            break;
                        case videodecoder::Property::AV_SOURCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(static_cast<int32_t>(g_av_source[this->id.value]));
                            break;
                        case videodecoder::Property::OUTPUT_FRAME_POOL_SIZE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_output_frame_pool_size[this->id.value]);
                            break;
                        case videodecoder::Property::OPERATIONAL_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_operational_mode[this->id.value]);
                            break;
                        case videodecoder::Property::LOW_LATENCY_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_low_latency[this->id.value]);
                            break;
                        case videodecoder::Property::DECODE_ERROR_POLICY:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_decode_error_policy[this->id.value]);
                            break;
                        case videodecoder::Property::SHA1_CALC:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_sha1_calc[this->id.value]);
                            break;
                        case videodecoder::Property::SECURE_VIDEO:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_support_secure[this->id.value]);
                            break;
                        case videodecoder::Property::METRIC_FRAMES_DECODED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_metrics_frame_decoded[this->id.value]);
                            break;
                        case videodecoder::Property::METRIC_DECODE_ERRORS:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_metrics_decode_errors[this->id.value]);
                            break;
                        case videodecoder::Property::METRIC_FRAMES_DROPPED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(g_metrics_frames_dropped[this->id.value]);
                            break;
                        default:
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = value;
                    return android::binder::Status::ok();
                }

                //TODO
                android::binder::Status VideoDecoder::getPropertyMulti(  std::vector<videodecoder::PropertyKVPair>* propertyKVList, bool* _aidl_return  )
                {
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::getState(  State* _aidl_return  )
                {
                    *_aidl_return = g_videodecoder_state[this->id.value];
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::open(  videodecoder::Codec codec, bool secure, const android::sp<videodecoder::IVideoDecoderControllerListener>& videoDecoderControllerListener, android::sp<videodecoder::IVideoDecoderController>* _aidl_return  )
                {
                    if(std::find(supportedCodecs.begin(), supportedCodecs.end(), codec) == supportedCodecs.end())
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(videoDecoderControllerListener == nullptr)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(g_videodecoder_state[this->id.value] != State::CLOSED)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from CLOSED to OPENING...\n";
                            g_videodecoder_state[this->id.value] = State::OPENING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for OPENING...\n";
                        _videoDecoderEventListener->onStateChanged(State::CLOSED, State::OPENING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from OPENING to READY...\n";
                            g_videodecoder_state[this->id.value] = State::READY;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for READY...\n";
                        _videoDecoderEventListener->onStateChanged(State::OPENING, State::READY);
                        state_cv.notify_all();
                    }
                    *_aidl_return = new VideoDecoderController(this->id);
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::close(  const android::sp<videodecoder::IVideoDecoderController>& videoDecoderController, bool* _aidl_return  )
                {
                    if(g_videodecoder_state[this->id.value] != State::READY)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from READY to CLOSING...\n";
                            g_videodecoder_state[this->id.value] = State::CLOSING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for CLOSING...\n";
                        _videoDecoderEventListener->onStateChanged(State::READY, State::CLOSING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from CLOSING to CLOSED...\n";
                            g_videodecoder_state[this->id.value] = State::CLOSED;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for CLOSED...\n";
                        _videoDecoderEventListener->onStateChanged(State::CLOSING, State::CLOSED);
                        state_cv.notify_all();
                    }
                    * _aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::registerEventListener(  const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return  )
                {
                    if(this->event_listener == true)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    *_aidl_return = true;
                    this->event_listener = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoder::unregisterEventListener(  const android::sp<videodecoder::IVideoDecoderEventListener>& videoDecoderEventListener, bool* _aidl_return  )
                {
                    if(this->event_listener ==false)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    *_aidl_return = true;
                    this->event_listener = false;
                    return android::binder::Status::ok();
                }
                
                VideoDecoderController::VideoDecoderController(IVideoDecoder::Id id)
                {
                    this->id = id;
                    printf("VideoDecoderController service initialized\n");
                }
                
                VideoDecoderController::~VideoDecoderController() 
                {
                    printf("VideoDecoderController service destroyed\n");
                }
                
                android::binder::Status VideoDecoderController::start()
                {
                    if(g_videodecoder_state[this->id.value] != State::READY)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from READY to STARTING...\n";
                            g_videodecoder_state[this->id.value] = State::STARTED;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for STARTING...\n";
                        _videoDecoderEventListener->onStateChanged(State::READY, State::STARTING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STARTING to STARTED...\n";
                            g_videodecoder_state[this->id.value] = State::STARTED;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for STARTED...\n";
                        _videoDecoderEventListener->onStateChanged(State::STARTING, State::STARTED);
                        state_cv.notify_all();
                    }
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::stop()
                {
                    if(g_videodecoder_state[this->id.value] != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STARTED to STOPPING...\n";
                            g_videodecoder_state[this->id.value] = State::STOPPING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for STOPPING...\n";
                        _videoDecoderEventListener->onStateChanged(State::STARTED, State::STOPPING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STOPPING to READY...\n";
                            g_videodecoder_state[this->id.value] = State::READY;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for READY...\n";
                        _videoDecoderEventListener->onStateChanged(State::STOPPING, State::READY);
                        state_cv.notify_all();
                    }
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::decodeBuffer(int64_t nsPresentationTime, int64_t bufferHandle, bool* _aidl_return)
                {
                    if(g_videodecoder_state[this->id.value] != State::STARTED)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    if(nsPresentationTime < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::flush(bool reset)
                {
                    android::binder::Status status;
                    if(g_videodecoder_state[this->id.value] != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from STARTED to FLUSHING...\n";
                            g_videodecoder_state[this->id.value] = State::FLUSHING;
                        }
                        //average wait time to simulate transitory state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        std::cout << "[SERVICE]::Calling onStateChanged for FLUSHING...\n";
                        _videoDecoderEventListener->onStateChanged(State::STARTED, State::FLUSHING);
                        state_cv.notify_all();
                    }
                    {
                        {
                            std::lock_guard <std::mutex> lock(state_mutex);
                            std::cout << "[SERVICE]::Changing state from FLUSHING to STARTED...\n";
                            g_videodecoder_state[this->id.value] = State::STARTED;
                        }
                        //average wait time to simulate target state change
                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                        std::cout << "[SERVICE]::Calling onStateChanged for STARTED...\n";
                        _videoDecoderEventListener->onStateChanged(State::FLUSHING, State::STARTED);
                        state_cv.notify_all();
                    }

                    if (reset)
                    {
                        {
                            {
                                std::lock_guard <std::mutex> lock(state_mutex);
                                std::cout << "[SERVICE]::Changing state from STARTED to READY...\n";
                                g_videodecoder_state[this->id.value] = State::READY;
                            }
                            //average wait time to simulate target state change
                            std::this_thread::sleep_for(std::chrono::milliseconds(250));
                            std::cout << "[SERVICE]::Calling onStateChanged for READY...\n";
                            _videoDecoderEventListener->onStateChanged(State::STARTED, State::READY);
                            state_cv.notify_all();
                        }
                    }
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::signalDiscontinuity()
                {
                    android::binder::Status status;
                    if(g_videodecoder_state[this->id.value] != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::signalEOS()
                {
                    android::binder::Status status;
                    if(g_videodecoder_state[this->id.value] != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    return android::binder::Status::ok();
                }
                
                android::binder::Status VideoDecoderController::parseCodecSpecificData(rdk::hal::videodecoder::CSDVideoFormat csdVideoFormat, int8_t codecData, bool* _aidl_return)
                {
                    return android::binder::Status::ok();
                }

                android::binder::Status VideoDecoderController::setProperty(videodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return)
                {
                    int32_t intvalue;
                    android::String16 strvalue;
                    *_aidl_return = true;
                    switch (property)
                    {
                        case videodecoder::Property::LOW_LATENCY_MODE:
                            if(g_videodecoder_state[this->id.value] != State::READY)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue  = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < 0 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_low_latency[this->id.value] = intvalue;
                            break;
                        case videodecoder::Property::AV_SOURCE:
                            if(g_videodecoder_state[this->id.value] != State::READY)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
                            if (intvalue < static_cast<int32_t>(AVSource::AUTO) || intvalue > static_cast<int32_t>(AVSource::COMPOSITE_5))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_av_source[this->id.value] = static_cast<AVSource>(intvalue);
                            break;
                        case videodecoder::Property::OPERATIONAL_MODE:
                            if(g_videodecoder_state[this->id.value] != State::READY && g_videodecoder_state[this->id.value] != State::STARTED)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
                            if (intvalue < static_cast<int32_t>(OperationalMode::TUNNELLED) || intvalue > static_cast<int32_t>(OperationalMode::GRAPHICS_TEXTURE))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }

                            g_operational_mode[this->id.value] = intvalue;
                            break;
                        case videodecoder::Property::DECODE_ERROR_POLICY:
                            if(g_videodecoder_state[this->id.value] != State::READY)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue  = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < 0 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_decode_error_policy[this->id.value] = intvalue;
                            break;
                        case videodecoder::Property::SHA1_CALC:
                            if(g_videodecoder_state[this->id.value] != State::READY && g_videodecoder_state[this->id.value] != State::STARTED)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < 0 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            g_sha1_calc[this->id.value] = intvalue;
                            break;
                        default:
                            *_aidl_return = false;
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    return android::binder::Status::ok();
                }
            }
        }
    }
}