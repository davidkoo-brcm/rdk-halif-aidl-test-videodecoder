#* ******************************************************************************
#*  If not stated otherwise in this file or this component's LICENSE
#*  file the following copyright and licenses apply:
#*
#*  Copyright 2025 RDK Management
#*
#*  Licensed under the Apache License, Version 2.0 (the License);
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*  http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an AS IS BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#*
#* ******************************************************************************

.PHONY: all

AVBUFFER_AIDL_LIB_NAME := avbufferaidl
VIDEODECODER_AIDL_LIB_NAME := videodecoderaidl
AUDIODECODER_AIDL_LIB_NAME := audiodecoderaidl
AVBUFFER_HELPER_LIB_NAME := avbufferhelper

AVBUFFER_AIDL_LIB := lib$(AVBUFFER_AIDL_LIB_NAME).so
VIDEODECODER_AIDL_LIB := lib$(VIDEODECODER_AIDL_LIB_NAME).so
AUDIODECODER_AIDL_LIB := lib$(AUDIODECODER_AIDL_LIB_NAME).so
AVBUFFERHELPER_LIB := lib$(AVBUFFER_HELPER_LIB_NAME).so

MOCK_SERVICE_BIN := MockVideoDecoderService

BUILD_DIR ?= $(CURDIR)/../build
AIDL_GEN_DIR ?= $(BUILD_DIR)/audiodecoder_gen/current
AV_BUFFER_AIDL_GEN_DIR ?= $(BUILD_DIR)/avbuffer_gen/current
VIDEO_DECODER_AIDL_GEN_DIR ?= $(BUILD_DIR)/videodecoder_gen/current

MOCK_SERVICE_DIR ?= $(CURDIR)/src

AVBUFFER_AIDL_SRC += ${AV_BUFFER_AIDL_GEN_DIR}/cpp/com/rdk/hal/avbuffer/HeapMetrics.cpp
AVBUFFER_AIDL_SRC += ${AV_BUFFER_AIDL_GEN_DIR}/cpp/com/rdk/hal/avbuffer/IAVBuffer.cpp
AVBUFFER_AIDL_SRC += ${AV_BUFFER_AIDL_GEN_DIR}/cpp/com/rdk/hal/avbuffer/IAVBufferSpaceListener.cpp
AVBUFFER_AIDL_SRC += ${AV_BUFFER_AIDL_GEN_DIR}/cpp/com/rdk/hal/avbuffer/Pool.cpp
AVBUFFER_AIDL_SRC += ${AV_BUFFER_AIDL_GEN_DIR}/cpp/com/rdk/hal/avbuffer/PoolMetrics.cpp

COMMON_AIDL_SRC +=${AIDL_GEN_DIR}/cpp/com/rdk/hal/AVSource.cpp
COMMON_AIDL_SRC +=${AIDL_GEN_DIR}/cpp/com/rdk/hal/HALError.cpp
COMMON_AIDL_SRC +=${AIDL_GEN_DIR}/cpp/com/rdk/hal/PropertyValue.cpp
COMMON_AIDL_SRC +=${AIDL_GEN_DIR}/cpp/com/rdk/hal/State.cpp

AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/Capabilities.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/ChannelType.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/Codec.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/CSDAudioFormat.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/ErrorCode.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/FrameMetadata.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderController.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderControllerListener.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoder.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderEventListener.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/IAudioDecoderManager.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/PCMFormat.cpp
AUDIODECODER_AIDL_SRC += ${AIDL_GEN_DIR}/cpp/com/rdk/hal/audiodecoder/Property.cpp


VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/Capabilities.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/CodecCapabilities.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/Codec.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/CSDVideoFormat.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/DynamicRange.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/ErrorCode.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/FrameMetadata.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderController.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderControllerListener.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoder.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderEventListener.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderManager.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/OperationalMode.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/PixelFormat.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/Property.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/PropertyKVPair.cpp
VIDEODECODER_AIDL_SRC += $(VIDEO_DECODER_AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/ScanType.cpp

MOCK_SERVICE_BIN_SRC := $(MOCK_SERVICE_DIR)/VideoDecoderService.cpp
MOCK_SERVICE_BIN_SRC += $(MOCK_SERVICE_DIR)/VideoDecoder.cpp

MOCK_SERVICE_INC_FLAGS := -I$(AIDL_GEN_DIR)/h
MOCK_SERVICE_INC_FLAGS += -I$(AV_BUFFER_AIDL_GEN_DIR)/h
MOCK_SERVICE_INC_FLAGS += -I$(VIDEO_DECODER_AIDL_GEN_DIR)/h
MOCK_SERVICE_INC_FLAGS += -I$(BUILD_DIR)/usr/include

MOCK_SERVICE_LD_LIBS := -lbinder -lutils -lbase

MOCK_SERVICE_LDFLAGS := -L$(BUILD_DIR)/usr/lib
MOCK_SERVICE_LDFLAGS += -Wl,-rpath-link=$(BUILD_DIR)/usr/lib

MOCK_SERVICE_CFLAGS := -Wall -Wextra -std=c++17 -ggdb

AVBUFFER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(AVBUFFER_AIDL_LIB)
AUDIODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(AUDIODECODER_AIDL_LIB)
VIDEODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(VIDEODECODER_AIDL_LIB)
MOCK_SERVICE_BIN := $(BUILD_DIR)/usr/lib/$(MOCK_SERVICE_BIN)

all: $(MOCK_SERVICE_BIN) $(AVBUFFER_AIDL_LIB) $(VIDEODECODER_AIDL_LIB) $(AUDIODECODER_AIDL_LIB)

$(AUDIODECODER_AIDL_LIB): $(AUDIODECODER_AIDL_SRC:%.cpp=%.o) $(COMMON_AIDL_SRC:%.cpp=%.o)
	g++ -shared $(MOCK_SERVICE_LDFLAGS) $^ -o $@ $(MOCK_SERVICE_LD_LIBS)

$(VIDEODECODER_AIDL_LIB): $(VIDEODECODER_AIDL_SRC:%.cpp=%.o) $(COMMON_AIDL_SRC:%.cpp=%.o)
	g++ -shared $(MOCK_SERVICE_LDFLAGS) $^ -o $@ $(MOCK_SERVICE_LD_LIBS)

$(AVBUFFER_AIDL_LIB): $(AVBUFFER_AIDL_SRC:%.cpp=%.o) $(COMMON_AIDL_SRC:%.cpp=%.o)
	g++ -shared $(MOCK_SERVICE_LDFLAGS) $^ -o $@ $(MOCK_SERVICE_LD_LIBS)

$(MOCK_SERVICE_BIN): $(VIDEODECODER_AIDL_LIB)
$(MOCK_SERVICE_BIN): $(MOCK_SERVICE_BIN_SRC:%.cpp=%.o)
	g++ $(MOCK_SERVICE_LDFLAGS) \
		$(MOCK_SERVICE_BIN_SRC:%.cpp=%.o) -o $@ \
		$(MOCK_SERVICE_LD_LIBS) \
		-lpthread \
		-l$(VIDEODECODER_AIDL_LIB_NAME)

%.o: %.cpp
	g++ $(MOCK_SERVICE_CFLAGS) $(MOCK_SERVICE_INC_FLAGS) -fPIC -c $< -o $@
