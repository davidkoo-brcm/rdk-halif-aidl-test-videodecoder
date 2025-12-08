# Test Stream specifications

L3 test media are local data files for the test suite to inject directly into the HAL.

They are read by the test suite, written into linear buffers and passed into the video decoder.

```bash

<CODEC>_<PRO>_<RES>_<DR>_<COL>_<FR>_<PIX>_<BITRate>_<description>.es (elementary stream file)
e.g.
H265_MAIN10_3840x2160_DR_HLG_DR_BT2020_25p_PIXEL_FORMAT_YCBCR420_18000.es

```

## Elementary stream files

- Single I-frame or IDR frame
- Single GOPs (closed)
- Multiple GOPs (open)
- Multiple GOPs (open and closed)

||Codec|Resolution|Dynamic Range|ColorSpace|Depth|Frame Rate|Pixel Formate|Bit Rate|
|-|-----|----------|------------|---------|-----|----------|-------------|--------|
|1|H.265|3840x2160 | SDR        |BT.709   |10 bit|50/60 | YUV420 |~18Mbps|

### Test Dimensions

| #|Test Dimension | Comments | Filename Part |
|--|---------------|----------|---------------|
| 1 | Codec | Test every codec which the platform capabilities report. <br> Test unsupported codecs to check for errors. | `<CODEC>` <br> Same as `com.rdk.hal.video.decoder.Codec` enum. <br> e.g. "CODEC_H265" |
| 2 | Profile/Level | For codecs that break down into profiles and levels. | `<PRO>` <br> e.g. "MAIN10" |
| 3 | Resolution | Test all Viper resolutions: <br> - 3840x2160 <br> - 2560x1440 <br> - 1920x1080 <br> - 1280x720 <br> - 960x540 <br> - 640x360 <br> - 320x240 <br> YouTube portrait mode videos from phones. <br> YouTube adds: <br> - 426x240 <br> - 7680x4320 (8K) | `<RES>` <br> HxW syntax. <br> e.g. "3840x2160" |
| 4 | Dynamic range | - SDR <br> - HLG <br> - HDR10 <br> - HDR10+ <br> - Dolby Vision | `<DR>` <br> Same as `com.rdk.hal.video.decoder.DynamicRange` enum. <br> e.g. "DR_HLG" |
| 5 | Color space | - BT.601 <br> - BT.709 <br> - BT.2020 | `<COL>` <br> Same as `com.rdk.hal.video.decoder.DynamicRange` enum. <br> e.g. "DR_BT2020" |
| 6 | Frame rate | Viper Frame rates (UK): 25, 50 <br> Viper Frame rates (US): ??? <br> Netflix Frame Rates: 23.976, 24, 25, 29.97, 30, 50, 59.94, 60 | `<FR>` <br> e.g. "23.976" |
| 7 | Frame types | Progressive and interlaced frame types. | `<FR>` <br> Suffix as "i" or "p" |
| 8 | Pixel format | - YCbCr4:4:4 <br> - YCbCr4:2:2 <br> - YCbCr4:2:0 | `<PIX>` <br> Same as `com.rdk.hal.video.decoder.PixelFormat` enum. <br> e.g. "PIXEL_FORMAT_YCBCR420" |
| 9 | Bit rates | Viper bitrates (UK): 18000, 12200, 5800, 4250, 1850, 1200, 500 <br> Check the maximum bit rates supported by codec/profile/level | `<BIT>` |
| 10 | Reference frames | Single reference frames are passed for decode and are expected to be output immediately. | |
| 11 | GOP structure | Open GOPs <br> Closed GOPs <br> Min and max M and N for each supported codec/profile/level | |
| 12 | Active Format Description | Test all AFD codes appear in FrameMetadata. <br> Complete list of AFD codes | |
| 13 | Resolution switching | Moving between different resolution encodings, without breaking GOP sequences. | |
| 14 | Aspect ratio switching | Picture aspect ratios must be reported correctly in FrameMetadata. <br> Pixel aspect ratios must be reported correctly in FrameMetadata. |
| 15 | End of stream | EOS signalling from application. <br> EOS in-band signalling. |
| 16 | Discontinuity/Data Loss | Loss of frames (incomplete GOPs). <br> Partial loss of frame data - check for decode errors. | |
| 17 | Secure/non-secure | Video frames passed in clear buffers. <br> Video frames passed in secure buffers. | |
| 18 | Low latency | Measure time improvement when low latency enabled. | |
| 19 | Flushing | Check that flush call follows state machine. <br> Check that flushed frames are returned. <br> Check that flushed frames never get output. | |
| 20 | Bad encoding (frame drops) | Check that errors, drops, metrics are updated as expected. <br> Corrupt video encoding. <br> Corrupt metadata encoding. | |

## Error streams info

AV VTS L4 includes playback of error (corrupted) streams to evaluate the error recovery capability of the product.

As part of this verification, various video error streams are generated and used during testing.

**Example:** Video Error Stream File Name

```bash

h265_2160p_hdr10_60.0f_BT2020_yuv422p_insert-noise-pframe_18M_a0_v3600_60sec.mp4

```

**Stream File Name Breakdown:**

|Codec|Resolution|Dynamic Range|Frame Rate|ColorSpace|Pixel Format|Error Type|Bit Rate|Audio frames|Video Frames|Duration|
|------|----------|------------|---------|----------|-------------|--------|------|----|----|----|
|H.265|3840x2160p | HDR10        |60|BT.2020   | YUV422p |insert-noise-pframe|18Mbps|0|3600|60sec|

**Supported Video Error Types:**

Below is the list of supported video error types used for AV VTS L4 recovery validation:

#### Basic / Soft Corruptions:

**bitflip-slice** – Randomly flips bits within a slice; may produce visual artifacts or minor decode warnings.

**overwrite-macroblock** – Replaces macroblock data with random bytes; can cause visible distortion in parts of the frame.

**insert-noise-pframe** – Injects noise into P-frames; affects motion-compensated frames, may propagate artifacts.

**light-fuzz-all** – Applies minor random bit modifications across the bitstream; usually produces subtle glitches.

**temporal-duplication** – Duplicates frames or slices, causing repeated frames and potential decoder warnings.

**drop-partial-slices** – Deletes parts of slices; may cause slice-level errors, missing blocks, or minor decoder logs.

**mixed-soft-corruption** – Combines several soft corruption techniques; produces a variety of artifacts and warnings.

#### NAL / Macroblock-Level Corruptions:

**nal-header-corruption** – Modifies NAL unit headers; can confuse decoder about slice type or payload length.

**macroblock-skipping** – Simulates skipped macroblocks within frames; results in missing blocks or distortion.

**motion-vector-corruption** – Alters motion vector data in P/B frames; introduces motion artifacts or mispredicted blocks.

**slice-reordering** – Changes the order of slices within a frame or GOP; may trigger “illegal reordering” warnings.

**partial-frame-truncation** – Cuts off frame data mid-frame; produces decoder warnings and visual corruption.

**nal-injection** – Inserts extra (fake) NAL units into the stream; may break reference frames or cause parsing errors.

**cross-slice-corruption** – Spreads slice corruption across multiple frames; can affect prediction and generate warnings.

#### Advanced / Structural Corruptions:

**sps-pps-corruption** – Modifies SPS/PPS parameter sets; may produce resolution/parameter warnings, sometimes prevents muxing.

**sei-corruption** – Corrupts SEI messages; usually parser warnings like “truncated SEI,” rarely affects playback.

**entropy-bit-flip** – Flips bits in CABAC/CAVLC-encoded entropy data; can cause decoder errors or macroblock corruption.

**gop-truncation-duplication** – Truncates or duplicates GOPs; may trigger reference-frame, reordering, or slice warnings.

**fake-reference-frame** – Replaces reference frame data with garbage; produces reference-related decode errors.

**slice-boundary-offset** – Moves slice boundaries within a frame; may break prediction chains and trigger warnings.

**pts-dts-corruption** – Alters PTS/DTS timestamps; can generate muxer/decoder warnings like non-monotonous DTS or PTS < DTS.
