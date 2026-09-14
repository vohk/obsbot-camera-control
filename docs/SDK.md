# OBSBOT SDK Analysis

The OBSBOT SDK (`sdk/`) is a closed-source library distributed as precompiled binaries and C++ headers. This document records what we know about it from inspecting the public headers and the unstripped binary.

## What's Distributed

- Three headers: `dev.hpp` (~4000 lines of declarations), `devs.hpp`, `comm.hpp`
- Compiled shared library: `libdev.so` (24MB, x86_64, ELF, debug symbols included, not stripped)
- One sample application: `sample_main.cpp`
- No license file, no copyright headers, no attribution notices

The library version reported in `comm.hpp` is 1.3.0. OBSBOT's download labeled "v2.1.0" contains byte-identical files (verified by SHA256).

## Internal Source Structure

Debug symbols in the unstripped binary reveal the original source tree:

```
libdev/
├── base64/          Base64 encode/decode
├── bsdiff/          Binary diff/patch (BSD 2-clause)
├── bzip2/           Compression (BSD-style license)
├── dev/
│   ├── dev.cpp, dev-info.cpp, dev-linux.cpp
│   ├── dev-mtp.cpp, dev-stream-linux.cpp, dev-upgrade.cpp
│   ├── dev-protocol-ai.cpp         # AI/tracking commands
│   ├── dev-protocol-camera.cpp     # Camera control commands
│   ├── dev-protocol-rm.cpp         # Remote management
│   ├── dev-protocol-sysmg.cpp      # System management
│   ├── dev-protocol-uvc-linux.cpp  # UVC extension unit protocol
│   ├── devs.cpp, devs-linux.cpp, devs-mdns.cpp
│   ├── av-capture-linux.cpp, bt-protocol-rm.cpp
│   └── uvc-protocol.cpp
├── ftp/             FTP client
├── kcp/             KCP reliable UDP transport
├── lzma/            Compression (public domain)
├── mdns/            mDNS/Zeroconf discovery
├── mtp/linux/       Full MTP/PTP/USB stack
├── package/         Firmware packaging and upgrade
├── protobuf/        Protobuf serialization (camera.pb.c, dev.pb.c)
└── util/            CRC, utilities
```

## What the SDK Actually Does

The library is a unified control interface for the entire OBSBOT product line — USB webcams, WiFi PTZ cameras, network converters. Most of its bulk is irrelevant for USB camera control:

| Component | Used for USB webcams? | Purpose |
|-----------|----------------------|---------|
| UVC XU protocol | **Yes** | Extension unit commands for AI tracking, camera settings |
| Device enumeration | **Yes** (but V4L2/udev does this too) | Find connected cameras |
| Protobuf serialization | **Yes** | Message encoding for XU commands |
| MTP stack | No | File transfer (backgrounds, firmware) |
| FTP client | No | Network camera file access |
| KCP/UDP | No | Reliable transport for WiFi cameras |
| mDNS | No | WiFi camera discovery |
| bsdiff/bzip2/lzma | No | Firmware delta updates |
| Bluetooth | No | BT device pairing |

For controlling a USB webcam, the SDK's job reduces to: write structured commands to UVC Extension Unit selectors and read status back. Everything else is overhead from the multi-product design.

## USB Work Modes

The binary references these camera USB modes, switchable via the SDK:

| Mode | Description |
|------|-------------|
| `UVC_UAC` | Normal webcam (video + audio) |
| `UVC_RNDIS` | Video + USB network interface |
| `RNDIS_ONLY` | Camera as pure network device |
| `MASS_STORAGE` | Camera as USB storage device |
| `MTP` | Media Transfer Protocol |
| `HOST` | Camera as USB host |
| `IDLE` | Idle state |

The RNDIS modes strongly suggest the camera runs an embedded Linux stack with USB gadget framework support.

## UVC Extension Unit (Direct Hardware Probe)

The OBSBOT Meet 2 exposes a vendor-specific UVC Extension Unit accessible without the SDK:

```
GUID: {9a1e7291-6843-4683-6d92-39bc7906ee49}
Unit ID: 2
Controls: 7 selectors, each 60 bytes, all GET+SET capable
```

Probed via standard `UVCIOC_CTRL_QUERY` ioctl:

| Selector | Idle State | Notes |
|----------|-----------|-------|
| 1 | zeros | Command channel |
| 2 | zeros | Command channel |
| 3 | zeros | Command channel |
| 4 | `01 00...` | State flag |
| 5 | zeros | Command channel |
| 6 | `24 00 00 02 02 00 01 01...` | Camera status (richest data) |
| 7 | `06 00...` | Product type or mode |

Selector 6 returns a packed struct with live camera state — AI mode, tracking status, feature enables, image settings. This is readable from any language that can do a V4L2 ioctl, no SDK required.

All standard camera controls (brightness, contrast, zoom, pan, tilt, exposure, white balance, focus) are exposed as normal V4L2 controls by the kernel `uvcvideo` driver and work without the SDK.

## Bundled Open Source Components

The binary contains code from several open source projects. No attribution is provided in the SDK distribution.

| Library | License | Attribution required? |
|---------|---------|----------------------|
| bsdiff | BSD 2-clause | Yes |
| bzip2 | BSD-style | Yes |
| LZMA SDK | Public domain | No |
| nanopb (protobuf) | zlib license | Yes |
| Boost | BSL-1.0 | Yes |

## Supported Devices (from SDK headers)

The `ObsbotProductType` enum in `dev.hpp`:

| Enum Value | Device |
|-----------|--------|
| `ObsbotProdTiny` | Tiny (original) |
| `ObsbotProdTiny4k` | Tiny 4K |
| `ObsbotProdTiny2` | Tiny 2 |
| `ObsbotProdTiny2Lite` | Tiny 2 Lite |
| `ObsbotProdTinySE` | Tiny SE |
| `ObsbotProdMeet` | Meet (original) |
| `ObsbotProdMeet4k` | Meet 4K |
| `ObsbotProdMeet2` | Meet 2 |
| `ObsbotProdMeetSE` | Meet SE |
| `ObsbotProdMe` | Me |
| `ObsbotProdTailAir` | Tail Air |
| `ObsbotProdTail2` | Tail 2 |
| `ObsbotProdTail2S` | Tail 2S |
| `ObsbotProdHDMIBox` | HDMI Box |
| `ObsbotProdNDIBox` | NDI Box |

## Meet SE HDR and asynchronous discovery

A connected Meet SE (firmware 4.6.4.1) was verified with SDK v1.0.2:
`cameraSetWdrR(DevWdrModeDol2TO1)` enabled HDR, and `DevWdrModeNone` disabled it.
`cameraStatus().tiny.hdr` reported off/on/off across the test. No special raw USB
command is needed. The status `hdr_support` was zero even when switching worked,
so it must not be treated as an authoritative capability gate for this model.

SDK USB enumeration runs asynchronously. Falling back to V4L2 immediately after
registering detection can prevent vendor controls from becoming available. The
GUI now allows 10 seconds for discovery before fallback and keeps discovery
alive so a late SDK detection can enable the full controls. SDK callbacks are
queued onto the Qt controller thread, and stale connection attempts are ignored.

Meet SE HDR supports up to 1080p at 60 fps; see the
[manufacturer FAQ](https://www.obsbot.jp/jp/obsbot-meet-se-full-hd-webcam/faq).
The SDK recommends at least three seconds between HDR switches.

### Meet SE preview intervals

Qt exposes each MJPEG resolution as a frame-rate range and selects its maximum
(1080p100 or 720p150 on Meet SE). An HDR status bit changing does not establish
that the output has HDR at those rates. The Meet SE preview now enumerates the
actual V4L2 discrete intervals and captures MJPEG directly, using `VIDIOC_S_PARM`
and checking `VIDIOC_G_PARM` before streaming. Other cameras keep the Qt path.
Decoded images pass through the same GPU effects and virtual-camera pipeline.

The default is 1080p60. The Meet SE menu offers only supported 30 and 60 fps
modes at each resolution, omitting fractional duplicates and uncommon intervals.
Enabling HDR while the negotiated camera rate exceeds 60 fails with
instructions to first start a lower-rate preview. This app cannot constrain a
separate application that opens the physical webcam at a higher rate.

Hardware checks observed approximately 59.94 decoded fps at negotiated 60 fps
and 29.97 decoded fps at negotiated 30 fps. Frames were decoded in memory only;
this verifies capture cadence, not the visual effectiveness of the HDR processing.
