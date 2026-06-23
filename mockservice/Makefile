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

.PHONY: all clean check-libs

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

# Use cross-compiler if available (set by toolchain), otherwise use native
CXX ?= g++
CXXFLAGS ?= -Wall -Wextra -std=c++17 -ggdb
MOCK_SERVICE_CXXFLAGS := $(CXXFLAGS) -Wall -Wextra -std=c++17 -ggdb

MOCK_SERVICE_DIR ?= $(CURDIR)/src

MOCK_SERVICE_BIN_SRC := $(MOCK_SERVICE_DIR)/VideoDecoderService.cpp
MOCK_SERVICE_BIN_SRC += $(MOCK_SERVICE_DIR)/VideoDecoder.cpp

# Use headers from CMake-generated locations in rdk-halif-aidl
RDK_HAL_DIR ?= $(BUILD_DIR)/../rdk-halif-aidl
COMMON_HEADERS ?= $(RDK_HAL_DIR)/common/current/include
AVBUFFER_HEADERS ?= $(RDK_HAL_DIR)/out/build/include/avbuffer/current/include
VIDEODECODER_HEADERS ?= $(RDK_HAL_DIR)/out/build/include/videodecoder/current/include
AUDIODECODER_HEADERS ?= $(RDK_HAL_DIR)/out/build/include/audiodecoder/current/include
BINDER_SDK_HEADERS ?= $(RDK_HAL_DIR)/out/build/include/binder_sdk

MOCK_SERVICE_INC_FLAGS := -I$(COMMON_HEADERS)
MOCK_SERVICE_INC_FLAGS += -I$(AVBUFFER_HEADERS)
MOCK_SERVICE_INC_FLAGS += -I$(VIDEODECODER_HEADERS)
MOCK_SERVICE_INC_FLAGS += -I$(AUDIODECODER_HEADERS)
MOCK_SERVICE_INC_FLAGS += -I$(BINDER_SDK_HEADERS)
MOCK_SERVICE_INC_FLAGS += -I$(BUILD_DIR)/usr/include

MOCK_SERVICE_LD_LIBS := -lbinder -lutils -lbase

MOCK_SERVICE_LDFLAGS := -L$(BUILD_DIR)/usr/lib
MOCK_SERVICE_LDFLAGS += -Wl,-rpath-link=$(BUILD_DIR)/usr/lib
MOCK_SERVICE_LDFLAGS += -Wl,--copy-dt-needed-entries

AVBUFFER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(AVBUFFER_AIDL_LIB)
AUDIODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(AUDIODECODER_AIDL_LIB)
VIDEODECODER_AIDL_LIB := $(BUILD_DIR)/usr/lib/$(VIDEODECODER_AIDL_LIB)
MOCK_SERVICE_BIN := $(BUILD_DIR)/bin/$(MOCK_SERVICE_BIN)

all: $(MOCK_SERVICE_BIN)

check-libs:
	@echo "Checking required runtime libraries in $(BUILD_DIR)/usr/lib"
	@test -e "$(BUILD_DIR)/usr/lib/libvideodecoder-vcurrent-cpp.so" || { echo "ERROR: missing $(BUILD_DIR)/usr/lib/libvideodecoder-vcurrent-cpp.so"; exit 1; }
	@test -e "$(BUILD_DIR)/usr/lib/libavbuffer-vcurrent-cpp.so" || { echo "ERROR: missing $(BUILD_DIR)/usr/lib/libavbuffer-vcurrent-cpp.so"; exit 1; }
	@test -e "$(BUILD_DIR)/usr/lib/libcommon-vcurrent-cpp.so" || { echo "ERROR: missing $(BUILD_DIR)/usr/lib/libcommon-vcurrent-cpp.so"; exit 1; }
	@test -e "$(BUILD_DIR)/usr/lib/libbinder.so" || { echo "ERROR: missing $(BUILD_DIR)/usr/lib/libbinder.so"; exit 1; }
	@test -e "$(BUILD_DIR)/usr/lib/libutils.so" || { echo "ERROR: missing $(BUILD_DIR)/usr/lib/libutils.so"; exit 1; }
	@test -e "$(BUILD_DIR)/usr/lib/libbase.so" || { echo "ERROR: missing $(BUILD_DIR)/usr/lib/libbase.so"; exit 1; }
	@echo "Library check passed"

$(MOCK_SERVICE_BIN): check-libs $(MOCK_SERVICE_BIN_SRC:%.cpp=%.o)
	@mkdir -p $(dir $@)
	$(CXX) $(MOCK_SERVICE_CXXFLAGS) $(MOCK_SERVICE_LDFLAGS) \
		$(MOCK_SERVICE_BIN_SRC:%.cpp=%.o) -o $@ \
		$(MOCK_SERVICE_LD_LIBS) \
		-lpthread \
		-lvideodecoder-vcurrent-cpp \
		-lcommon-vcurrent-cpp \
		-lavbuffer-vcurrent-cpp

%.o: %.cpp
	$(CXX) $(MOCK_SERVICE_CXXFLAGS) $(MOCK_SERVICE_INC_FLAGS) -fPIC -Wno-error -w -c $< -o $@


clean:
	rm -f $(MOCK_SERVICE_BIN_SRC:%.cpp=%.o)
	rm -f $(MOCK_SERVICE_BIN)
