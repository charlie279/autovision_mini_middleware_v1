# V2.6 GStreamer Camera Pipeline 新增命令与 Git 步骤

## 1. 版本定位

V2.6 基于 V2.5 DDS + SHM mixed-route 增量实现 GStreamer Camera Pipeline。V2.5 已保留 `metadata over DDS + raw payload over POSIX SHM FramePool` 主线；V2.6 新增媒体框架入口，使 Camera 数据既可以走原生 V4L2，也可以走 GStreamer `v4l2src -> capsfilter -> queue -> appsink` 进入 AutoVision `SensorFrame`。

V2.6 不改变 V2.4 raw FastDDS、V2.5 DDS+SHM 的现有链路；新增代码默认 dependency-free 编译，未启用 GStreamer 时明确输出 `NOT_COMPILED`。

## 2. 新增文件

```text
include/gstreamer_pipeline_config.hpp
include/gstreamer_camera_adapter.hpp
src/gstreamer_pipeline_config.cpp
src/gstreamer_camera_adapter.cpp
src/gstreamer_camera_capture_node.cpp
src/gstreamer_encode_stream_node.cpp
examples/cpp/26_gstreamer_pipeline_config.cpp
scripts/run_gstreamer_camera_capture.sh
scripts/run_gstreamer_encode_stream.sh
docs/V2_6_新增命令与Git步骤.md
docs/validation_v2_6_gstreamer_camera_pipeline.md
docs/local_validation/v2_6_gstreamer_camera_pipeline_container_validation.md
```

## 3. 修改文件

```text
CMakeLists.txt
README.md
examples/Makefile
examples/README.md
scripts/benchmark_gstreamer_camera.sh
```

## 4. 默认环境编译

```bash
cd ~/projects/autovision_mini_middleware_v1
chmod +x scripts/*.sh
./scripts/build.sh
```

预期新增目标：

```text
Built target avm_gstreamer
Built target gstreamer_camera_capture_node
Built target gstreamer_encode_stream_node
[build] done
```

## 5. 主链路回归

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

预期：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## 6. examples 验证

```bash
cd examples
make bin/23_fastdds_packet_codec bin/24_camera_fastdds_packet bin/25_dds_shm_frame_meta bin/26_gstreamer_pipeline_config
make run EXAMPLE=23_fastdds_packet_codec
make run EXAMPLE=24_camera_fastdds_packet
make run EXAMPLE=25_dds_shm_frame_meta
make run EXAMPLE=26_gstreamer_pipeline_config
```

预期：

```text
[23_fastdds_packet_codec] ... result=PASS
[24_camera_fastdds_packet] ... old_512k_qos=EXPECTED_FAIL result=PASS
[25_dds_shm_frame_meta] metadata_bytes=208 raw_payload_bytes=614400 size_errors=0 payload_errors=0 result=PASS
[26_gstreamer_pipeline_config] raw_payload_bytes=614400 capture_ok=1 encode_ok=1 size_ok=1 result=PASS
```

## 7. dependency-free GStreamer fallback 验证

```bash
./scripts/benchmark_gstreamer_camera.sh /dev/video0 5 640 480 30 yuyv
cat logs/benchmark_v2_6_gstreamer_camera/gstreamer_camera_capture.csv
cat logs/benchmark_v2_6_gstreamer_camera/gstreamer_camera_synthetic_fallback.csv
```

当前无 GStreamer dev package 容器预期：

```text
capture,false,/dev/video0,yuyv,640,480,30,5,0,0,0,0,0,0,NOT_COMPILED
capture,0,/dev/video0,yuyv,640,480,30,5,5,0,0,...,PASS
```

## 8. GStreamer ON 编译

安装依赖后：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly

./scripts/build.sh -DAVM_ENABLE_GSTREAMER=ON
```

本容器当前等待项：`pkg-config --exists gstreamer-1.0 gstreamer-app-1.0` 不通过，因此 real GStreamer ON build 等待用户 VMware/真实开发环境复测。

## 9. GStreamer Camera Capture 验证

```bash
./scripts/run_gstreamer_camera_capture.sh /dev/video0 300 640 480 30 yuyv
cat logs/benchmark_v2_6_gstreamer_camera/gstreamer_camera_capture.csv
```

GStreamer ON + USB Camera 预期：

```text
role,compiled,device,format,width,height,fps,frames_requested,captured,size_errors,payload_errors,mean_interval_ms,p95_interval_ms,last_crc32,status
capture,true,/dev/video0,yuyv,640,480,30,300,300,0,0,...,...,...,PASS
```

## 10. GStreamer Encode/Stream 验证

```bash
./scripts/run_gstreamer_encode_stream.sh synthetic /dev/video0 120 640 480 30 yuyv h264 logs/gstreamer_output.h264
cat logs/benchmark_v2_6_gstreamer_camera/gstreamer_encode_stream.csv
```

GStreamer ON + x264enc 插件可用时预期：

```text
encode,true,synthetic,/dev/video0,yuyv,640,480,30,120,120,logs/gstreamer_output.h264,<nonzero>,PASS
```

## 11. Git 分阶段提交建议

```bash
git checkout v2.5-dds-shm-mixed-route
git pull origin v2.5-dds-shm-mixed-route
git checkout -b v2.6-gstreamer-camera-pipeline

git add include/gstreamer_pipeline_config.hpp src/gstreamer_pipeline_config.cpp
git commit -m "v2.6-step1: add dependency-free GStreamer pipeline config helpers"

git add include/gstreamer_camera_adapter.hpp src/gstreamer_camera_adapter.cpp CMakeLists.txt
git commit -m "v2.6-step2: add optional GStreamer appsink camera adapter"

git add src/gstreamer_camera_capture_node.cpp src/gstreamer_encode_stream_node.cpp
git commit -m "v2.6-step3: add GStreamer capture and encode stream demo nodes"

git add scripts/run_gstreamer_camera_capture.sh scripts/run_gstreamer_encode_stream.sh scripts/benchmark_gstreamer_camera.sh
git commit -m "v2.6-step4: add GStreamer camera benchmark scripts"

git add examples/cpp/26_gstreamer_pipeline_config.cpp examples/Makefile examples/README.md
git commit -m "v2.6-step5: add GStreamer pipeline config example"

git add docs/V2_6_新增命令与Git步骤.md docs/validation_v2_6_gstreamer_camera_pipeline.md docs/local_validation/v2_6_gstreamer_camera_pipeline_container_validation.md README.md
git commit -m "v2.6-step6: document GStreamer camera pipeline validation"

git push -u origin v2.6-gstreamer-camera-pipeline
```

## 12. 不应提交

```text
build/
build_gst_on/
logs/
assets/
examples/bin/
examples/logs/
*.h264
*.h265
*.raw
*.yuyv
compile_commands.json
```
