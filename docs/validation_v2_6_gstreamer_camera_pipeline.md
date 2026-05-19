# AutoVision Mini Middleware V2.6 GStreamer Camera Pipeline Validation

## 1. 目标

在 V2.5 DDS + SHM mixed-route 基线上，新增 GStreamer camera/media pipeline：

```text
USB Camera /dev/video0
  -> v4l2src
  -> video/x-raw capsfilter
  -> queue
  -> appsink name=appsink
  -> GStreamerCameraAdapter
  -> AutoVision SensorFrame
```

并新增 appsrc 编码/流媒体 demo：

```text
SensorFrame synthetic / GStreamer camera
  -> appsrc name=appsrc
  -> videoconvert
  -> x264enc / x265enc / raw filesink
```

## 2. 新增能力

| 能力 | 文件 | 状态 |
|---|---|---|
| Pipeline 字符串构造 | `gstreamer_pipeline_config.hpp/cpp` | 已实现，dependency-free 已验证 |
| GStreamer appsink 采集适配器 | `GStreamerCameraAdapter` | 已实现，真实 GStreamer ON 待环境验证 |
| Camera capture 节点 | `gstreamer_camera_capture_node` | 已实现，fallback 已验证 |
| Encode/stream 节点 | `gstreamer_encode_stream_node` | 已实现，fallback 已验证 |
| example 26 | `26_gstreamer_pipeline_config` | 已验证 PASS |
| scripts | `benchmark_gstreamer_camera.sh` 等 | 已验证 fallback |

## 3. 当前容器已验证

```text
./scripts/build.sh：PASS
./scripts/run_all_vm.sh：PASS，final_status=NORMAL，media/preprocess/npu=120/120/120
examples/23_fastdds_packet_codec：PASS
examples/24_camera_fastdds_packet：PASS
examples/25_dds_shm_frame_meta：PASS
examples/26_gstreamer_pipeline_config：PASS
benchmark_gstreamer_camera.sh：GStreamer capture NOT_COMPILED + synthetic fallback PASS
run_gstreamer_encode_stream.sh：NOT_COMPILED fallback 已验证
```

## 4. 当前容器具体验证输出

### 4.1 主链路回归

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

### 4.2 examples 23/24/25/26

```text
[23_fastdds_packet_codec] encoded_bytes=160096 kind=lidar seq=7 payload=160000 decode=OK validate=OK result=PASS
[24_camera_fastdds_packet] yuyv_payload=614400 yuyv_packet=614496 rgb_payload=921600 rgb_packet=921696 v24_qos_yuyv=OK v24_qos_rgb=OK old_512k_qos=EXPECTED_FAIL result=PASS
[25_dds_shm_frame_meta] metadata_bytes=208 raw_payload_bytes=614400 size_errors=0 payload_errors=0 result=PASS
[26_gstreamer_pipeline_config] raw_payload_bytes=614400 capture_ok=1 encode_ok=1 size_ok=1 result=PASS
```

### 4.3 GStreamer fallback

```text
[gstreamer_camera_capture_node] status=not_compiled: rebuild with -DAVM_ENABLE_GSTREAMER=ON and install gstreamer-1.0/app/video development packages
[gstreamer_camera_capture_node] captured=5 size_errors=0 payload_errors=0 ... result=PASS
```

CSV：

```text
capture,false,/dev/video0,yuyv,640,480,30,5,0,0,0,0,0,0,NOT_COMPILED
capture,0,/dev/video0,yuyv,640,480,30,5,5,0,0,...,PASS
```

### 4.4 GStreamer ON 尝试

当前容器缺少开发包：

```text
Package 'gstreamer-1.0', required by 'virtual:world', not found
Package 'gstreamer-app-1.0', required by 'virtual:world', not found
```

因此 `-DAVM_ENABLE_GSTREAMER=ON` 真实编译等待用户 VMware/真实开发环境。

## 5. 等待用户环境验证

```text
1. 安装 GStreamer dev packages 后 -DAVM_ENABLE_GSTREAMER=ON 编译。
2. USB Camera /dev/video0 300 帧 appsink capture 验证。
3. appsrc + x264enc/x265enc 文件输出验证。
4. V4L2 原生采集 vs GStreamer appsink 采集 vs GStreamer encode 的 latency/fps/CPU 对比。
5. --use-dmabuf 请求 v4l2src io-mode=dmabuf 的板端行为验证。
```

## 6. 验收标准

### 6.1 dependency-free 验收

```text
默认构建通过
主链路 final_status=NORMAL
examples 23/24/25/26 均 PASS
未启用 GStreamer 时 capture/encode 节点输出 NOT_COMPILED，不伪造 real pipeline PASS
synthetic fallback 捕获 5 帧 PASS
```

### 6.2 GStreamer ON 验收

```text
-DAVM_ENABLE_GSTREAMER=ON 编译通过
run_gstreamer_camera_capture.sh /dev/video0 300 640 480 30 yuyv 输出 captured=300 size_errors=0 payload_errors=0 PASS
run_gstreamer_encode_stream.sh synthetic ... h264 输出 pushed=120 output_bytes>0 PASS
custom pipeline 必须包含 appsink name=appsink 或 appsrc name=appsrc，否则返回 INVALID_PIPELINE
```

## 7. 边界说明

```text
1. V2.6 不删除 V2.4/V2.5 链路。
2. V2.6 的 GStreamer 模块默认 dependency-free；真实 GStreamer 需 -DAVM_ENABLE_GSTREAMER=ON。
3. --use-dmabuf 当前只请求 v4l2src io-mode=dmabuf；appsink 侧仍 map/copy 到 SensorFrame.data。
4. true DMA-BUF fd handoff、跨进程 fd 生命周期管理、zero-copy 编码器接入等待 V3.x 板端版本。
5. x264enc/x265enc 插件可用性取决于用户本机 GStreamer runtime 插件安装。
```
