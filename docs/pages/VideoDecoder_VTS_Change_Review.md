# VideoDecoder VTS Change Review

## Latest test results

Latest recorded decoder L3 device run: **2026-11-09 21:52:56–22:03:30**.

Platform: Broadcom Xione, xione-bcm-flex2. Profile: `profiles/hfp-unified_xione.yaml`.

| Total | Passed | Failed | Skipped | Duration |
| ---: | ---: | ---: | ---: | ---: |
| 94 | **22** | **2** | 70 | 640.985 s |

Sequential and concurrent Tunnel NonSecure tests pass for basic decoding, CODEC, AdaptiveBitrate, HDRModes, FrameRate, LowLatency, GOPStructure, Discontinuity, EOS, Flush, and FlushReset.

The failing tests are `VDecode_Sequential_Tunnel_NonSecure_Error` and `VDecode_Concurrent_Tunnel_NonSecure_Error`. The truncated H264/HEVC inputs produce no NEXUS decode-error callbacks. Sequential output contains all twelve pictures, exceeding the configured error-case bound.

Seventy tests skip for unsupported conditions including NonTunnel and Secure. The captured decoder run uses the headless HAL path (`tunnelled=0`, `appDisplayManagement=1`) despite Tunnel in the names.

This is the latest recorded decoder-suite result, not a rerun after the current shared helper changes. The 2026-11-10 VideoSink run is a separate suite.

## Current change scope

Current changes cover capability selection, metadata hints, source offsets, flush/Error inputs, and shared helper behavior. Original encoded assets and Xione profiles are retained. After changing the shared configuration structure, rebuild its consumers in VideoSink, AVClock, PlaneControl and AV L4 and relink the unified VTS. Deploy matching binaries and YAML.

See the [VideoSink review](../../../videosink/docs/pages/VideoSink_VTS_Change_Review.md) for sink execution.

## Dynamic range capability selection

The explicit HAL range list is authoritative. An empty range list uses HFP for that field only. Full HFP fallback also checks dynamic range. Unsupported streams are excluded per decoder.

Location: [src/vts_videodecoder_util.cpp:760–771](../../src/vts_videodecoder_util.cpp#L760) · `checkDecoderSupportsConfig()`

```cpp
auto supportsDynamicRange = [&](const std::vector<videodecoder::DynamicRange>& ranges,
                                const char* source) -> bool
{
    if (std::find(ranges.begin(), ranges.end(), testConfig.dynamicRange) == ranges.end())
    {
        UT_LOG_WARNING("Decoder [%d] skipping stream [%s]: dynamic range [%s] unsupported by %s",
                       decoderId.value, testConfig.streamName.c_str(),
                       videodecoder::toString(testConfig.dynamicRange).c_str(), source);
        return false;
    }
    return true;
};
```

Match the requested range and log the capability source.

Location: [src/vts_videodecoder_util.cpp:857–865](../../src/vts_videodecoder_util.cpp#L857) · `checkDecoderSupportsConfig()`

```cpp
// Some implementations omit this list. Use HFP for that field only;
// keep checking the remaining configuration against HAL capabilities.
const bool useHfpDynamicRanges = capabilities->supportedDynamicRanges.empty();
if (!supportsDynamicRange(useHfpDynamicRanges ? hfpDecoder.supportedDynamicRanges
                                            : capabilities->supportedDynamicRanges,
                          useHfpDynamicRanges ? "HFP (HAL list unavailable)" : "HAL"))
{
    return false;
}
```

Use HFP only when the HAL range list is empty.

## Metadata expectations and application hints

YAML metadata describes expected output. omit_stream_metadata defaults to false; true suppresses application colorimetry, mastering-display and content-light hints while keeping output assertions.

Location: [src/vts_videodecoder.h:280–284](../../src/vts_videodecoder.h#L280) · `videoDecoderTestConfiguration_t`

```cpp
videodecoder::Colorimetry colorimetry;
// Output expectations remain populated even when application hints are omitted.
bool omitStreamMetadata{false};
std::optional<MasteringDisplayInfo> masteringDisplayInfo;
std::optional<ContentLightLevel> contentLightLevel;
```

Separate expected metadata from hint suppression.

Location: [src/vts_videodecoder_util.cpp:1408–1410](../../src/vts_videodecoder_util.cpp#L1408) · `vts_videodecoder_parse_testconfig()`

```cpp
const string omitStreamMetadata = prefix + "omit_stream_metadata";
testDetails.omitStreamMetadata = ut_kvp_fieldPresent(kvpTestInstance, omitStreamMetadata.c_str())
    && ut_kvp_getBoolField(kvpTestInstance, omitStreamMetadata.c_str());
```

An absent field leaves hints enabled.

Location: [src/vts_videodecoder_test.cpp:234–246](../../src/vts_videodecoder_test.cpp#L234) · `buildAndApplyStreamConfig()`

```cpp
Colorimetry unknownColorimetry{};
if (!mTestConfiguration.omitStreamMetadata && mTestConfiguration.colorimetry != unknownColorimetry)
{
    streamConfig.colorimetry = mTestConfiguration.colorimetry;
}
if (!mTestConfiguration.omitStreamMetadata && mTestConfiguration.masteringDisplayInfo.has_value())
{
    streamConfig.masteringDisplayInfo = mTestConfiguration.masteringDisplayInfo;
}
if (!mTestConfiguration.omitStreamMetadata && mTestConfiguration.contentLightLevel.has_value())
{
    streamConfig.contentLightLevel = mTestConfiguration.contentLightLevel;
}
```

Apply the flag to three hint types. Resolution, frame rate, aspect ratio and mid-stream overrides keep their own paths.

Location: [videodecoder_test_configuration/vts_video_hevc_streams.yaml:1277–1279](../../videodecoder_test_configuration/vts_video_hevc_streams.yaml#L1277) · `COLORIMETRY_NOSET`

```yaml
H265_HIGH_1920x1080_DR_HDR10_BT2020_30p_PIXEL_FORMAT_YCBCR420_COLORIMETRY_NOSET:
  input: H265_HIGH_1920x1080_DR_HDR10_BT2020_ST2084_30p_PIXEL_FORMAT_YCBCR420_COLORIMETRY.es
  omit_stream_metadata: true # No application hints; validate metadata from the bitstream.
```

Test encoded HDR metadata without matching application hints.

| Input | Expected output |
| --- | --- |
| H264 720x480 / 1280x720; H264 / HEVC ABR | LIMITED, BT709 matrix, UNKNOWN transfer/primaries. |
| HEVC COLORIMETRY_NOSET | LIMITED, BT2020_NCL, SMPTE2084_PQ, BT2020. |
| HEVC MDI_NOSET | Same mastering-display values as the matching MDI asset. |
| HEVC CLL_NOSET | maxCLL=1000, maxFALL=400. |
| VP9 ABR | LIMITED and BT709 matrix/transfer/primaries. |

## Absolute source offsets and error indices

frame_offsets stores zero-based original byte positions corresponding to frame_size/PTS. Repeated offsets enable replay. Truncation preserves later source boundaries. VP9 offsets address payloads excluding IVF headers.

Location: [src/vts_videodecoder_util.cpp:1527–1542](../../src/vts_videodecoder_util.cpp#L1527) · `vts_videodecoder_parse_testconfig()`

```cpp
const string frameOffsets = prefix + "frame_offsets";
if (ut_kvp_fieldPresent(kvpTestInstance, frameOffsets.c_str()))
{
    const int32_t offsetCount = ut_kvp_getListCount(kvpTestInstance, frameOffsets.c_str());
    if (offsetCount != frameCount)
    {
        UT_ASSERT_TRUE(false) << "frame_offsets and frame_size must have the same length for "
                              << testDetails.streamName;
        continue;
    }
    for (int32_t j = 0; j < offsetCount; ++j)
    {
        const string offsetKey = frameOffsets + "." + to_string(j);
        testDetails.frameOffsets.push_back(ut_kvp_getUInt64Field(kvpTestInstance, offsetKey.c_str()));
    }
}
```

Require matching offset/buffer counts.

Location: [src/vts_videodecoder_test_utils.cpp:286–304](../../src/vts_videodecoder_test_utils.cpp#L286) · `videoDecoderTest::makeFrame()`

```cpp
uint64_t offset = 0;
size_t frameIndex = 0;
for (const auto& [size, pts, isErrorFrame] : mTestConfiguration.frameDetails)
{
    if (!mTestConfiguration.frameOffsets.empty())
    {
        offset = mTestConfiguration.frameOffsets[frameIndex];
    }
    ++frameIndex;
    if (size <= 0 || offset > fileSize || static_cast<uint64_t>(size) > fileSize - offset)
    {
        UT_LOG_ERROR("Frame size at offset %llu exceeds file size %llu", offset + size, fileSize);
        return false;
    }
    mFrameData.emplace_back(offset, size, pts, isErrorFrame);
    UT_LOG_DEBUG("Codec:%d Frame from frameDetails: offset=%llu, size=%u, pts=%lld, errorFrame=%d",
                mTestConfiguration.codec, offset, size, pts, isErrorFrame);
    offset += size;
}
```

Select explicit ranges and use subtraction to check bounds without unsigned addition overflow.

Location: [src/vts_videodecoder_util.cpp:1490–1510](../../src/vts_videodecoder_util.cpp#L1490) · `vts_videodecoder_parse_testconfig()`

```cpp
// Error indices address submitted buffers, including separate parameter
// sets, rather than no_of_frames (which counts output pictures).
int32_t frameCount = ut_kvp_getListCount(kvpTestInstance, frameSize.c_str());
// Error frames
if (ut_kvp_fieldPresent(kvpTestInstance, errorFrames.c_str()))
{
    int32_t count = ut_kvp_getListCount(kvpTestInstance, errorFrames.c_str());
    for (int32_t j = 0; j < count; j++)
    {
        string error = errorFrames + "." + to_string(j);
        uint32_t frameNum = ut_kvp_getUInt32Field(kvpTestInstance, error.c_str());
        if (frameNum >= 1 && frameNum <= static_cast<uint32_t>(frameCount))
        {
            errorFrameNum.push_back(frameNum);
        }
        else
        {
            UT_LOG_WARNING("Invalid error frame index: %d, max: %d", frameNum, frameCount);
        }
    }
}
```

error_frames uses one-based input-buffer indices including parameter sets, bounded by frame_size count.

## Flush and reset inputs

H264/HEVC/VP9 FLUSH and FLUSH_RESET fixtures replay original headers/keyframes with picture PTS shifted by 1,000,000,000 ns. Twelve distinct picture PTS values remain. Each control point executes once before its first submitted buffer.

Location: [src/vts_videodecoder_test.cpp:309–314](../../src/vts_videodecoder_test.cpp#L309) · `videoDecoderTest::startDecoding()`

```cpp
void videoDecoderTest::startDecoding()
{
    int64_t videoBufferHandle = 0;
    mEOSignaledCount= 0; //before start decode reset the count to 0
    bool flushSignaled = false;
    bool flushResetSignaled = false;
```

Initialize one-time control flags per run.

Location: [src/vts_videodecoder_test.cpp:387–402](../../src/vts_videodecoder_test.cpp#L387) · `videoDecoderTest::startDecoding()`

```cpp
// Parameter sets can share the random-access frame's PTS. Execute
// each control point once, before feeding the first parameter set.
if (!flushSignaled && mTestConfiguration.flushPts == framePTS)
{
    flushSignaled = true;
    UT_LOG_INFO("Flushing decoder at PTS:%lld", framePTS);
    VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, mVideoDecoderController, false, true);
    VIDEODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
}
if (!flushResetSignaled && mTestConfiguration.flushResetPts == framePTS)
{
    flushResetSignaled = true;
    UT_LOG_INFO("Flush reset at PTS:%lld", framePTS);
    VIDEODECODER_FLUSH_SUCCESS(mDecoderHandle, mVideoDecoderController, true, true);
    VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
}
```

Non-reset flush signals discontinuity; reset flush restarts the decoder.

Location: [videodecoder_test_configuration/vts_video_h264_streams.yaml:860–880](../../videodecoder_test_configuration/vts_video_h264_streams.yaml#L860) · `H264 FLUSH frame_offsets`

```yaml
frame_offsets:
  - 0
  - 733
  - 764
  - 775
  - 1507
  - 110225
  - 113122
  - 114198
  - 115241
  - 116342
  - 117540
  - 118632
  - 119347
  - 123387
  - 0
  - 733
  - 764
  - 775
  - 1507
  - 110225
```

Offsets repeat from zero at random-access restart. Flush PTS is at line 845; reset PTS is at line 938.

## Error input configuration

H264 truncates input buffers 14/15 and HEVC truncates 13/14 while preserving source offsets. Each declares two expected decode errors. Positive error reporting and output-picture bounds remain asserted.

Location: [videodecoder_test_configuration/vts_video_h264_streams.yaml:644–646](../../videodecoder_test_configuration/vts_video_h264_streams.yaml#L644) · `H264 Error fixture`

```yaml
no_of_frames: 12
decodeErrors: 2
framesDropped: 0
```

Declare twelve pictures and two expected errors.

Location: [videodecoder_test_configuration/vts_video_h264_streams.yaml:670–672](../../videodecoder_test_configuration/vts_video_h264_streams.yaml#L670) · `H264 error_frames`

```yaml
error_frames:
  - 14
  - 15
```

Indices identify expected damaged buffers; flags do not alter encoded bytes.

## Optional resolution changes

resolution_changes is an optional ABR list.

Location: [src/vts_videodecoder_util.cpp:1633–1636](../../src/vts_videodecoder_util.cpp#L1633) · `vts_videodecoder_parse_testconfig()`

```cpp
// In-band resolution/bitrate ladder breakpoints (AdaptiveBitrate streams only)
string resolutionChanges = prefix + "resolution_changes";
int32_t resolutionChangeCount = ut_kvp_fieldPresent(kvpTestInstance, resolutionChanges.c_str())
    ? ut_kvp_getListCount(kvpTestInstance, resolutionChanges.c_str()) : 0;
```

Check field presence before reading list length; an absent list has zero entries.

## Shared helper behavior for VideoSink

Keep completed decoder sessions for attached-sink EOS. Tunnel underflow tests can pause input until both sink listeners observe starvation.

Location: [src/vts_videodecoder_test.cpp:451–461](../../src/vts_videodecoder_test.cpp#L451) · `videoDecoderTest::startDecoding()`

```cpp
    // Preserve the completed decoder session until the attached sink
    // has reported presentation EOS. Decoder-only tests retain their
    // existing stop/start state-transition coverage.
    if (!mIsSinkAttached)
    {
        VIDEODECODER_STOP_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
        VIDEODECODER_START_SUCCESS(mDecoderHandle, mVideoDecoderController, true);
    }
    // Exit the loop and not process any more frames after EOS is signaled.
    break;
}
```

Only decoder-only tests stop/start at this EOS point.

Location: [src/vts_videodecoder.h:1760](../../src/vts_videodecoder.h#L1760) · `videoDecoderTest::setInputPauseHook()`

```cpp
void setInputPauseHook(std::function<bool()> hook) { mInputPauseHook = std::move(hook); }
```

Store the optional hook; its private member is at line 1772.

Location: [src/vts_videodecoder_test.cpp:463–469](../../src/vts_videodecoder_test.cpp#L463) · `videoDecoderTest::startDecoding()`

```cpp
if (mIsSinkAttached && mTunnelMode &&
    mTestConfiguration.sink_underflow_pts == framePTS)
{
    // Pause the producer until both listeners observe real starvation.
    const bool underflowObserved = mInputPauseHook && mInputPauseHook();
    UT_ASSERT_TRUE(underflowObserved) << "No sink underflow while decoder input was paused";
}
```

Require actual underflow observation at the configured pause PTS.
