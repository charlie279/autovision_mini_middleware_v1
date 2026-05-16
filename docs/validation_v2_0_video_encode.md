# AutoVision Mini Middleware V2.0 Video Encode Validation

## 版本定位

V2.0 在 V1.9 Feature Pack 基线上新增视频编码子系统：

- `VideoEncoder` 抽象接口与 `VideoEncoderFactory` 工厂；
- `SoftVideoEncoder`，当前 Linux 容器采用 `ffmpeg CLI` 生成真实 H.264/H.265 Annex-B 裸流，未安装 FFmpeg 开发头文件时不依赖 libavcodec-dev；
- `V4l2M2mEncoder` stub，保留 Linux V4L2 M2M 硬编接口边界；
- `MppVideoEncoder` stub，保留 Rockchip MPP / DMA-BUF 输入接口边界；
- `encode_sink_node` 独立进程，消费 `ShmFramePool + FrameMeta Ring`，输出 `.h264/.h265` 和 CSV；
- `examples/cpp/20_video_encode_benchmark.cpp`；
- `scripts/run_encode_pipeline.sh` 与 `scripts/benchmark_encode.sh`。

## 当前 Linux 容器验证结果

### 1. 主工程编译

命令：

```bash
./scripts/build.sh
```

结果：通过。新增 target `encode_sink_node` 已成功生成。

### 2. V1.9 文件输入链路回归

命令：

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

结果：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

### 3. V2.0 H.264 编码链路

命令：

```bash
./scripts/run_encode_pipeline.sh soft h264 30
ffprobe -v error -show_entries stream=codec_name,width,height -of default=noprint_wrappers=1 logs/encode_output.h264
```

结果摘要：

```text
[encode_sink_node] started backend=soft_ffmpeg_cli codec=H264 frames=30 output=logs/encode_output.h264
[encode_sink_node] done frames_consumed=30 encode_ok=30 empty_polls=0
codec_name=h264
width=640
height=480
```

### 4. V2.0 H.265 编码链路

命令：

```bash
./scripts/run_encode_pipeline.sh soft h265 10
ffprobe -v error -show_entries stream=codec_name,width,height -of default=noprint_wrappers=1 logs/encode_output.h265
```

结果摘要：

```text
[encode_sink_node] started backend=soft_ffmpeg_cli codec=H265 frames=10 output=logs/encode_output.h265
[encode_sink_node] done frames_consumed=10 encode_ok=10 empty_polls=0
codec_name=hevc
width=640
height=480
```

### 5. examples 编译与 example 20

命令：

```bash
cd examples
make all
make run EXAMPLE=20_video_encode_benchmark
```

结果：通过。`20_video_encode_benchmark` 可输出 soft、mpp_stub、v4l2m2m 的 benchmark CSV。

### 6. 编码 benchmark 脚本

命令：

```bash
./scripts/benchmark_encode.sh 10 640 480
```

结果：通过，输出：

```text
logs/benchmark_v2_0/encode_benchmark.csv
```

## 真实性边界

1. 当前容器没有 `/dev/video0`，因此真实 USB Camera 路径仍需在你的 VMware Ubuntu 或开发板侧验证。
2. 当前容器没有 Rockchip MPP SDK 和真实 V4L2 M2M 编码设备，因此 `mpp` 与 `v4l2m2m` 为可编译、可运行的接口 stub；不宣称已完成板端硬编。
3. 当前 `soft` backend 在存在 `ffmpeg` 命令时会生成真实 H.264/H.265 裸流；若目标环境没有 `ffmpeg`，会退回 soft stub，只用于流程验证。
4. 若你在 VMware Ubuntu 安装了 FFmpeg 开发包，后续可以把 `SoftVideoEncoder` 内部替换为 libavcodec API 实现，外部 `VideoEncoder` 接口、`encode_sink_node` 和脚本无需变化。
