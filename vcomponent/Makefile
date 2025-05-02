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

VIDEODECODER_AIDL_LIB_NAME := videodecoderaidl

VIDEODECODER_AIDL_LIB := lib$(VIDEODECODER_AIDL_LIB_NAME).so

EMULATE_SERVICE_BIN := EmulateVideoDecoderService

BUILD_DIR ?= $(CURDIR)/../build

AIDL_GEN_DIR ?= $(BUILD_DIR)/videodecoder_gen/current

EMULATE_SERVICE_DIR := $(CURDIR)/src

VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/AVSource.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/PropertyValue.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/State.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/Capabilities.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/CodecCapabilities.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/Codec.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/CSDVideoFormat.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/DynamicRange.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/ErrorCode.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/FrameMetadata.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderController.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderControllerListener.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoder.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderEventListener.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/IVideoDecoderManager.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/OperationalMode.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/PixelFormat.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/Property.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/PropertyKVPair.cpp
VIDEODECODER_AIDL_SRC += $(AIDL_GEN_DIR)/cpp/com/rdk/hal/videodecoder/ScanType.cpp

EMULATE_SERVICE_BIN_SRC := $(EMULATE_SERVICE_DIR)/VideoDecoderService.cpp
EMULATE_SERVICE_BIN_SRC += $(EMULATE_SERVICE_DIR)/VideoDecoder.cpp

EMULATE_SERVICE_INC_FLAGS := -I$(AIDL_GEN_DIR)/h
EMULATE_SERVICE_INC_FLAGS += -I$(BUILD_DIR)/usr/include

EMULATE_SERVICE_LD_LIBS := -lbinder -lutils -lbase

EMULATE_SERVICE_LDFLAGS := -L$(BUILD_DIR)/usr/lib
EMULATE_SERVICE_LDFLAGS += -Wl,-rpath-link=$(BUILD_DIR)/usr/lib

EMULATE_SERVICE_CFLAGS := -Wall -Wextra -std=c++17 -ggdb

VIDEODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(VIDEODECODER_AIDL_LIB)
EMULATE_SERVICE_BIN := $(BUILD_DIR)/usr/lib/$(EMULATE_SERVICE_BIN)

all: $(EMULATE_SERVICE_BIN) $(VIDEODECODER_AIDL_LIB)

$(VIDEODECODER_AIDL_LIB): $(VIDEODECODER_AIDL_SRC:%.cpp=%.o)
	g++ -shared $(EMULATE_SERVICE_LDFLAGS) $^ -o $@ $(EMULATE_SERVICE_LD_LIBS)

$(EMULATE_SERVICE_BIN): $(VIDEODECODER_AIDL_LIB)
$(EMULATE_SERVICE_BIN): $(EMULATE_SERVICE_BIN_SRC:%.cpp=%.o)
	g++ $(EMULATE_SERVICE_LDFLAGS) \
		$(EMULATE_SERVICE_BIN_SRC:%.cpp=%.o) -o $@ \
		$(EMULATE_SERVICE_LD_LIBS) \
		-lpthread \
		-l$(VIDEODECODER_AIDL_LIB_NAME)

%.o: %.cpp
	g++ $(EMULATE_SERVICE_CFLAGS) $(EMULATE_SERVICE_INC_FLAGS) -fPIC -c $< -o $@
