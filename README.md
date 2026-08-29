# gst-nvwrapdec

Hardware video decode for browsers (and anything GStreamer) on Jetson.

Browsers on Jetson decode video on the CPU while NVDEC sits idle. Reason:
`nvv4l2decoder` only outputs NVMM memory, generic apps want plain
`video/x-raw`, so decodebin skips the hw decoder and falls back to software.

The manual fix is well known (`nvv4l2decoder ! nvvidconv ! video/x-raw`) but
browsers pick their own decoders. This plugin wraps that pipeline in a GstBin
registered as a normal decoder ranked above nvv4l2decoder, so autopluggers
pick it up. Tested on Orin Nano Super, JetPack 7.2.1 (r39.2.1).

## Install

```sh
sudo apt install -y build-essential pkg-config libgstreamer1.0-dev
./build.sh
sudo cp libgstnvwrapdec.so /usr/lib/aarch64-linux-gnu/gstreamer-1.0/
rm -rf ~/.cache/gstreamer-1.0
gst-inspect-1.0 nvwrapdec
```

Uninstall: delete the .so, clear the cache dir again.

## Verify

```sh
GST_DEBUG=decodebin3:5 gst-launch-1.0 playbin3 uri=file:///path/to/video.webm \
  video-sink=fakesink 2>&1 | grep 'Trying decoder'
```

Want: `Trying decoder <nvwrapdec0>`. Then play a video in a browser and check
NVDEC: `sudo cat /sys/kernel/debug/clk/clk_summary | grep nvdec`
(separate engine, doesn't show in any "GPU %". jtop shows it.)

## Numbers

Orin Nano Super, MAXN, Epiphany:

| test                | native pipeline | browser + nvwrapdec |
|---------------------|-----------------|---------------------|
| 1080p               | ~idle           | works, near free    |
| 4K24 VP9            | 15% CPU         | ~55% CPU, smooth    |
| 4K60 AV1 10bit HDR  | 35% CPU         | plays, drops frames |
| 8K30 HEVC           | 1.33x realtime  | stalls              |

The gap is the NVMM->system copy. Free at 1080p, over budget at 4K60. For
that you need dmabuf zero-copy or hole-punching (not this repo).

That AV1 file fails in nvgstplayer-1.0 and NVIDIA's ffmpeg has no AV1 on
Orin, so afaict this is the only way to play AV1 in a browser on this
platform.

## Caveats

- 10bit/HDR gets converted to 8bit NV12. Plays, HDR metadata lost.
- WebKit's sandbox can't see tegra driver paths, so nv plugins fail to load
  inside it. Testing: `WEBKIT_DISABLE_SANDBOX_THIS_IS_DANGEROUS=1`. For real
  use, bind-mount the tegra dirs instead.
- `VIDIOC_CROPCAP` warnings are harmless driver noise.

MIT. Do whatever.
