# Video Decoder VTS Change Review

Date: 2026-11-09  
Platform: Broadcom Xione, `xione-bcm-flex2`  
Status: Local experimental changes for review by the VTS maintainers

The released Video Decoder VTS requires maintainer authorization to change.
The changes below were made locally to investigate L3 failures. They are not
an approved update to the released test suite. This record describes the
observed problems, the local implementation, and the evidence needed for
maintainer review. It does not establish certification against the released,
unmodified VTS.

## Scope and revision references

The component changes are recorded in local commit `fac40c6`:
`Fix video decoder capability checks and stream test fixtures`.
Its parent, `1dae09f`, identifies the local comparison baseline; it is not a
claim about the official upstream release baseline.

A related test-framework change is recorded separately in ut-core commit
`aea8837`: `Track header dependencies for test objects and rebuild legacy
objects`.

HAL changes are outside the VTS patch. The associated local HAL commit is
`4fdcb8b`, with ticket `SERXIONE-8909`. It includes decoder error policy and
notification handling, BCMV EOS draining, flush synchronization, and removal
of temporary diagnostics. Hardware results below were obtained with both
HAL and VTS changes, before the final diagnostic-log removal.

The selected top-level profile was
`profiles/hfp-unified_xione.yaml`. Its videodecoder include is
[the component Xione profile](../../profiles/hfp-videodecoder_xione.yaml).
The profile describes decoder 0 as supporting HDR and decoder 1 as SDR-only.
The changes in this review do not modify either profile.

## Files changed

Paths in this table are relative to the videodecoder component unless noted.

| File | Local change |
| --- | --- |
| [src/vts_videodecoder.h](../../src/vts_videodecoder.h) | Add optional application-metadata suppression and explicit source offsets to the test configuration. |
| [src/vts_videodecoder_util.cpp](../../src/vts_videodecoder_util.cpp) | Check dynamic range; parse new fields; validate error-buffer indices; guard an optional configuration list. |
| [src/vts_videodecoder_test.cpp](../../src/vts_videodecoder_test.cpp) | Separate metadata expectations from application hints; execute each flush control point once. |
| [src/vts_videodecoder_test_utils.cpp](../../src/vts_videodecoder_test_utils.cpp) | Read explicit source ranges and validate their bounds. |
| [vts_video_h264_streams.yaml](../../videodecoder_test_configuration/vts_video_h264_streams.yaml) | Correct color expectations and define restartable flush and truncated Error inputs. |
| [vts_video_hevc_streams.yaml](../../videodecoder_test_configuration/vts_video_hevc_streams.yaml) | Correct HDR and ABR expectations and define restartable flush and truncated Error inputs. |
| [vts_video_vp9_streams.yaml](../../videodecoder_test_configuration/vts_video_vp9_streams.yaml) | Correct ABR color expectations and define restartable flush inputs. |
| [ut-core/Makefile](../../../../ut-core/Makefile) | Generate header dependencies and rebuild legacy objects; this file is outside the component. |

## Dynamic range capability checks

### Observed problem

`checkDecoderSupportsConfig()` checked codec, output pixel format,
resolution, frame rate, and security, but did not check dynamic range.
Consequently, decoder 1 received six HEVC HDR10 configurations despite
advertising SDR-only support. In the captured run, each configuration
produced no output pictures and twelve input-drop error notifications.

### Local change

The requested `testConfig.dynamicRange` must appear in the selected
`supportedDynamicRanges` list.

- An explicit HAL list is authoritative. HFP cannot override a HAL rejection.
- If the HAL omits the dynamic range list, only that field falls back to HFP;
  the remaining configuration still uses the HAL capability checks.
- If capabilities are unavailable, or an existing full-HFP fallback applies,
  the HFP dynamic range list is checked as well.
- A missing or empty effective list does not imply support.

Unsupported stream configurations are skipped for the individual decoder,
with the decoder ID, stream name, requested range, and capability source
logged. This is not necessarily a Google Test skip for the enclosing test;
other supported streams continue to run.

No general codec-profile or bit-depth filter was added. The stream
configuration does not currently specify an explicit codec profile.
Maintainers should review whether more complete profile matching is needed,
including for 10-bit SDR inputs.

## Bitstream metadata expectations and application hints

### Observed problem

Several expectations treated absent application configuration as absent
output metadata. The HEVC `COLORIMETRY_NOSET`, `MDI_NOSET`, and `CLL_NOSET`
cases use the same encoded assets as their corresponding non-NOSET cases.
Those assets contain the metadata being tested.

The H264 720x480 and 1280x720 inputs also contain LIMITED range and a BT709
matrix. The previous default expectation was an entirely UNKNOWN tuple.
ABR ladder inputs similarly signaled color information that was absent from
the YAML expectations.

### Local change

The YAML metadata fields remain output expectations. A new optional field,
`omit_stream_metadata`, controls whether the application seeds three matching
fields in `VideoDecoderStreamConfig`:

```yaml
omit_stream_metadata: true
```

It defaults to `false`. When true, `buildAndApplyStreamConfig()` omits
`colorimetry`, `masteringDisplayInfo`, and `contentLightLevel`, while frame
validation still checks the expected bitstream values. It does not suppress
resolution, frame rate, pixel aspect ratio, or mid-stream metadata overrides.

| Configuration | Updated expectation |
| --- | --- |
| H264 720x480 and 1280x720 | LIMITED range, BT709 matrix, UNKNOWN transfer and primaries. |
| HEVC `COLORIMETRY_NOSET` | LIMITED range, BT2020_NCL matrix, SMPTE2084_PQ transfer, BT2020 primaries. |
| HEVC `MDI_NOSET` | Mastering-display values matching the same encoded MDI asset and its non-NOSET configuration. |
| HEVC `CLL_NOSET` | `maxCLL=1000`, `maxFALL=400`. |
| H264 and HEVC ABR | LIMITED range, BT709 matrix, UNKNOWN transfer and primaries. |
| VP9 ABR | LIMITED range, BT709 matrix, transfer, and primaries. |

FFprobe inspection confirmed the H264/HEVC metadata and LIMITED/BT709 matrix
throughout the ABR resolution ladders. The installed FFprobe did not report
VP9 transfer and primaries explicitly; those two expectations follow the
VP9 BT709 interpretation used by the HAL and observed output. Maintainers
should verify that mapping against their authoritative stream description.

The metadata equality assertions were retained. The NOSET cases still
exercise output metadata without the corresponding application hints.

## Explicit source offsets for input buffers

### Observed problem

The H264 and HEVC loaders derived the next source position by summing
`frame_size`. In Error cases, selected sizes were shortened to truncate
slices. That also shifted all later reads into the omitted bytes, so later
buffers no longer started at their original NAL boundaries.

### Local change

The new optional `frame_offsets` list specifies zero-based byte offsets in
the original input file. Each entry corresponds to the same entry in
`frame_size` and `PTS`.

```yaml
# Illustrative ranges, not a complete test configuration.
frame_offsets:
  - 123387
  - 124647
frame_size:
  - 110
  - 190
```

For this H264 example, shortening the first slice to 110 bytes no longer
moves the next source read from 124647 to 123497.

The parser requires offset and size counts to match. The loader rejects
nonpositive sizes, offsets beyond the file, and ranges larger than the
remaining file contents. The range check avoids unsigned addition overflow.

When explicit offsets are present, the same source-range path can be used
for VP9. Its offsets point to encoded frame payloads, excluding IVF file and
per-frame headers. The first frame of the tested IVF asset starts at byte
44. Repeated or nonmonotonic source offsets are allowed for replay fixtures.
Without explicit offsets, the existing codec-specific loading path remains.

`error_frames` indices are one-based input-buffer indices, including
separate parameter-set buffers. They are now checked against the number of
`frame_size` entries rather than `no_of_frames`, which counts pictures.
The flag records an expected damaged input; it does not itself corrupt data.

## Flush and flush reset inputs

### Observed problem

The previous fixtures flushed the decoder and immediately continued with
interdependent pictures from the middle of the same GOP. They did not
supply a new random-access point. NEXUS reported input drops, and some
runs had no post-flush picture carrying discontinuity metadata.

### Local change

The H264, HEVC, and VP9 `_734_FLUSH` and `_734_FLUSH_RESET` fixtures retain
an initial stream prefix, then replay the original headers and keyframe
using explicit source offsets. Replayed picture PTS values are shifted by
1,000,000,000 ns, and the flush control point is placed at that boundary.
The fixtures retain twelve distinct picture PTS values in total.

Parameter sets and the first picture can share the boundary PTS.
`startDecoding()` now executes each flush or flush-reset control point only
once per run, before submitting the first buffer at that PTS. A non-reset
flush is followed by `signalDiscontinuity()`; a reset flush is followed by
`start()`, as before.

These fixtures test recovery with valid random-access input. They do not
establish that the HAL can resume arbitrary dependent frames after a flush.
The VTS checks for discontinuity, EOS notifications, frame-count bounds,
and unexpected errors remain enabled.

The separate HAL change stops and joins its output worker before flushing
and resetting the PTS queue. It samples outstanding errors before the input
counter resets, flushes the playpump before the decoder, and restarts the
worker for a non-reset flush. The observed flush passes depend on this HAL
behavior as well as the corrected VTS inputs.

## Error input corrections and remaining failures

The H264 Error fixture now uses original source offsets while truncating
input buffers 14 and 15. The HEVC fixture similarly truncates buffers 13
and 14. Their `decodeErrors` setting was changed from four and three,
respectively, to two, matching the number of deliberately shortened slices.
The source assets themselves were not modified.

That setting participates in the existing expected-picture calculation,
`no_of_frames - decodeErrors`, and requires positive error reporting. It
therefore still assumes two damaged pictures are suppressed. The actual
relationship between a damaged input, reference-picture propagation, and
hardware-reported errors needs maintainer review.

FFmpeg detected decoding errors in both reconstructed truncated streams.
However, the latest NEXUS run reported zero decode-error and input-drop
counts for these fixtures, sent no error callbacks, and output all twelve
pictures in the sequential runs. HAL policy 1 was confirmed at decoder
start. The two Error tests consequently still fail their frame-count bound
and positive-error checks.

The error assertions were not removed to obtain a passing result. A next
step is to establish a reproducible damaged fixture that the target decoder
actually detects, compare policy 0 and policy 1, and confirm both suppression
and event delivery. FFmpeg detection alone is insufficient to establish
that NEXUS must report the same corruption. PTS association after actual
hardware frame drops also remains a validation requirement.

## Optional resolution change configuration

`resolution_changes` is an optional ABR list. The parser now checks that the
field exists before calling `ut_kvp_getListCount()`. Ordinary streams no
longer emit misleading missing-key errors for this optional field. Existing
ABR entries are still parsed normally.

## Shared header dependency tracking

Adding configuration fields changes the layout of
`videoDecoderTestConfiguration_t`. Besides videodecoder, the shared header
is included by videosink, avclock, planecontrol, and AV L4 code. Rebuilding
only the videodecoder objects can leave incompatible users of that layout
in the unified executable.

The observed `free(): invalid pointer` crash stopped after a unified rebuild
with dependency tracking, consistent with mixed structure layouts. A crash
backtrace was not captured to independently establish that cause.

The separate ut-core Makefile change generates `.d` dependency files with
`-MMD -MP -MF ... -MT ...` for C and C++ objects. It covers both objects under
the build directory and objects stored beside sources. Existing objects
without a `.d` file are rebuilt once. Subsequent builds follow the generated
header prerequisites.

Maintainers adopting the structure additions must rebuild all consumers
and relink the unified VTS executable. Updated binaries and YAML need to be
deployed together. The binary source assets do not require replacement for
these local fixture changes. Old released parsers do not implement the new
offset and metadata-suppression fields.

## Validation and observed results

Local checks included fourteen dynamic-range capability scenarios, actual
frame-loader execution against eight offset-based fixtures and invalid
ranges, and actual flush control flow tested with real worker threads and
a simulated NEXUS boundary. FFmpeg decoded both sides of all six flush
fixtures without errors and detected errors in the two truncated fixtures.
YAML parsing and Git whitespace checks passed.

The hardware test selection was `VTS_L3_VideoDecoder*` using the Xione unified
profile. These are combined HAL and locally modified VTS results:

| Investigation point | Passed | Failed | Skipped | Total |
| --- | ---: | ---: | ---: | ---: |
| Initial captured run | 4 | 20 | 70 | 94 |
| After initial HAL, metadata, and capability work | 16 | 8 | 70 | 94 |
| Latest captured run on 2026-11-09 | 22 | 2 | 70 | 94 |

The latest run took 640.985 seconds. Sequential and concurrent Tunnel
NonSecure tests passed for basic decoding, CODEC, AdaptiveBitrate, HDRModes,
FrameRate, LowLatency, GOPStructure, Discontinuity, EOS, Flush, and
FlushReset. Only these two tests failed:

- `VTS_L3_VideoDecoder.VDecode_Sequential_Tunnel_NonSecure_Error`
- `VTS_L3_VideoDecoder.VDecode_Concurrent_Tunnel_NonSecure_Error`

The seventy skips reflect unsupported execution conditions, including
non-tunnel and secure operation, and do not validate those modes. Although
the passing tests have Tunnel in their names, the captured L3 runs used the
HAL headless path with `tunnelled=0` and `appDisplayManagement=1`. The results
do not establish display-sink integration coverage.

The captured evidence was stored at:

- `/local/home/davidkoo/codex/vts/vts_1.log` for the initial run.
- `/local/home/davidkoo/codex/vts/vts.log` for the latest VTS run.
- `/local/home/davidkoo/codex/vts/av-hal-service.log` for its HAL run.

The latest VTS timestamps span 21:52:56 through 22:03:30 on 2026-11-09.
These filenames are reused during debugging; preserve the matching logs
with any maintainer review submission. The latest hardware run preceded
final removal of HAL diagnostic prints, so the final logging cleanup has
cross-compiler syntax validation but no subsequent recorded device run.

## Items for VTS maintainer review

1. Confirm the authoritative stream metadata and the intended distinction
   between output expectations and application hints, including NOSET cases.
2. Review dynamic-range fallback policy and whether codec-profile or bit-depth
   checks should accompany it.
3. Review `frame_offsets`, input-buffer error indices, and the random-access
   flush fixture semantics as additions to the released test configuration.
4. Establish hardware-detectable Error fixtures and the required relationship
   between damaged pictures, output bounds, and reported errors.
5. Review the ut-core dependency fix independently, then rerun the released
   suite with accepted changes and preserve matching HAL/VTS evidence.

The component's [contribution guidance](../../CONTRIBUTING.md) provides the
maintainer submission route. The local commits are review references only;
release ownership and acceptance remain with the VTS maintainers.
