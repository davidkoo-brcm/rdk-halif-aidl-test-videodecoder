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
 sp<IVideoDecoderManager> service;

  std::vector<IVideoDecoder::Id>            decoderIds;
  std::vector<sp<IVideoDecoder>>            decoders;
  std::vector<sp<IVideoDecoderController>>  controllers;
UT_status_t getVideoDecoderService();

UT_status_t getVideoDecoderService() {
    service = android::interface_cast<IVideoDecoderManager>(
        defaultServiceManager()->getService(String16(IVideoDecoderManager::serviceName().c_str())));
    if (service == nullptr) {
        printf("Service not available\n");
        return UT_STATUS_FAILURE;
    }
    return UT_STATUS_OK;
}

int main(int argc, char **argv)
{
    UT_status_t result;
    UT_init(argc, argv);
    if(getVideoDecoderService()!=UT_STATUS_OK)
    {
        return UT_STATUS_FAILURE;
    }

    result = UT_run_tests();
    return result;
}