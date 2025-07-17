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
 * @defgroup Video_Decoder_Halif_Test_Init Video Decoder HALIF Tests Init File
 * @{
 * @parblock
 *
 * ### Initialization functions for Video Decoder VTS HALIF AIDL tests :
 *
 *
 *
 * **Pre-Conditions:**  None@n
 * **Dependencies:** None@n
 *
 * Ref to API Definition specification documentation :
 *
 * @endparblock
 */


/**
 * @file vts_videodecoder_init.cpp
 *
 */

 #include "vts_videodecoder.h"

sp<IVideoDecoderManager> gVideoDecoderManager;

 std::vector<IVideoDecoder::Id>            decoderIds;
 std::vector<sp<IVideoDecoder>>            decoders;
 std::vector<sp<IVideoDecoderController>>  controllers;

void *gVideoDecoderTestConfiguration;

UT_status_t VTS_VideoDecoder_init()
{

    if(getVideoDecoderService() != UT_STATUS_OK)
    {
        return UT_STATUS_FAILURE;
    }
    if(vts_videodecoder_parseVideoDecoderHFP() != UT_STATUS_OK)
    {
        UT_LOG_ERROR("Failed to process the HFP file");
        return UT_STATUS_FAILURE;
    }

    gVideoDecoderTestConfiguration = vts_kvp_create_instance(VIDEO_DECODER_TEST_CONFIG_PATH);

    return UT_STATUS_OK;
}

void VTS_VideoDecoder_destroy()
{
    vts_kvp_destroy_instance(gVideoDecoderTestConfiguration);
}

/** @} */ // End of Video_Decoder_Halif_Test_Init
/** @} */ // End of Video_Decoder_Halif_Test
/** @} */ // End of Video_Decoder
/** @} */ // End of HPK