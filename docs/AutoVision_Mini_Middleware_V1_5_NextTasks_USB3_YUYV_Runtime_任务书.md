# AutoVision Mini Middleware V1.5 下一步任务书

**版本名称**：V1.5-USB3.1-YUYV-Camera-Runtime-Patch  
**适用工程**：`charlie279/autovision_mini_middleware_v1`  
**适用环境**：Windows 主机 + VMware Ubuntu 22.04/24.04 + 外接 USB UVC 摄像头  
**当前状态**：V1 文件输入闭环已经完成；USB 摄像头在 VMware USB 3.1 设置下完成 YUYV 640×480 mmap 功能验收；下一步进入工程代码实现阶段。  
**本任务书目标**：不重复 V1 已实现内容，只说明从当前状态继续推进 V1.5 需要新增和修改的任务、代码、测试和 Git 提交步骤。

---

## 0. 当前结论

当前技术路线恢复为 YUYV 主线：

```text
USB UVC Camera /dev/video0
    ↓
V4L2 ioctl + mmap
    ↓
YUYV 640×480@30fps
    ↓
CameraAdapterV4L2::readFrame()
    ↓
YUYV -> RGB888
    ↓
SensorFrame RGB888
    ↓
media_node
    ↓
POSIX SHM FramePool + FrameMeta Ring
    ↓
preprocess_node
    ↓
TensorPool + TensorMeta Ring
    ↓
npu_stub_node with InferenceRuntime
    ↓
safety_monitor + control_service
```

MJPEG 只作为后续 fallback，不进入本次第一批代码补丁。原因是 YUYV 已经通过 mmap 功能验收，第一阶段应避免引入 JPEG 解码依赖，优先把真实 V4L2 Camera 接入已有主链路。

---

## 1. 本阶段不再重复的 V1 内容

V1 已完成内容不再作为本任务书重点：

```text
FileAdapter / LidarSimAdapter
media_node 文件输入链路
ShmFramePool
ShmRingBuffer
preprocess_node RGB resize + normalize
npu_stub_node 原始 stub
shared_status
safety_monitor
control_service / control_client
examples 00~10
run_all_vm.sh 文件输入闭环
```

本阶段只做 V1.5 增量：

```text
1. CameraAdapterV4L2 真实 YUYV mmap。
2. CameraAdapter example 11。
3. Camera pipeline 脚本。
4. npu_stub_node Runtime 抽象化。
5. DMA-BUF 设计文档。
6. V1.5 验收报告模板。
```

---

## 2. 新增和修改文件清单

把补丁包解压到工程根目录后，涉及文件如下：

```text
CMakeLists.txt                                  更新
include/camera_adapter_v4l2.hpp                 更新
src/camera_adapter_v4l2.cpp                     更新
src/media_node.cpp                              更新

include/runtime_config.hpp                      新增
include/runtime_output.hpp                      新增
include/inference_runtime.hpp                   新增
include/dummy_runtime.hpp                       新增
include/rknn_runtime_stub.hpp                   新增
include/runtime_factory.hpp                     新增
src/dummy_runtime.cpp                           新增
src/rknn_runtime_stub.cpp                       新增
src/runtime_factory.cpp                         新增
src/npu_stub_node.cpp                           更新

examples/cpp/11_camera_adapter_v4l2.cpp         新增
examples/Makefile                               更新

scripts/check_usb_camera_v1_5.sh                新增
scripts/run_camera_pipeline.sh                  新增

docs/camera_usb3_1_retest_report.md             新增
docs/dmabuf_zero_copy_design.md                 新增
docs/performance_report_v1_5.md                 新增
docs/AutoVision_Mini_Middleware_V1_5_NextTasks_USB3_YUYV_Runtime_任务书.md  新增
```

---

## 3. 阶段 A：建立 V1.5 开发分支

进入工程：

```bash
cd ~/projects/autovision_mini_middleware_v1
```

确认工作区状态：

```bash
git status
```

建立分支：

```bash
git checkout main
git pull
git checkout -b v1.5-v4l2-runtime
```

如果 `git pull` 因网络或 SSH 问题失败，先跳过 pull，但要保证当前 main 是你已验证过的 V1 最新代码。

---

## 4. 阶段 B：复制补丁文件到工程

假设补丁包为：

```text
AutoVision_Mini_Middleware_V1_5_YUYV_Runtime_CodePatch.zip
```

建议放置位置：

```text
~/projects/autovision_mini_middleware_v1/
```

执行：

```bash
cd ~/projects/autovision_mini_middleware_v1
cp /mnt/data/AutoVision_Mini_Middleware_V1_5_YUYV_Runtime_CodePatch.zip .
unzip -o AutoVision_Mini_Middleware_V1_5_YUYV_Runtime_CodePatch.zip
chmod +x scripts/*.sh
```

如果你是在 Windows 下载 zip 后复制到 Ubuntu，可放到工程根目录后执行：

```bash
unzip -o AutoVision_Mini_Middleware_V1_5_YUYV_Runtime_CodePatch.zip
chmod +x scripts/*.sh
```

解压后检查：

```bash
git status --short
```

应看到上述新增和修改文件。

---

## 5. 阶段 C：先提交 Camera 检测记录

如果你要把摄像头复测日志纳入 Git，可执行：

```bash
./scripts/check_usb_camera_v1_5.sh /dev/video0
```

日志默认输出到：

```text
logs/camera_check/
```

注意：`logs/` 通常在 `.gitignore` 中，不建议直接提交大日志。建议只提交文档报告：

```bash
git add docs/camera_usb3_1_retest_report.md
git commit -m "v1.5-step1: document USB3.1 YUYV camera validation"
```

---

## 6. 阶段 D：实现 CameraAdapterV4L2 YUYV mmap

### 6.1 修改目标

更新：

```text
include/camera_adapter_v4l2.hpp
src/camera_adapter_v4l2.cpp
```

核心功能：

```text
open /dev/video0
VIDIOC_QUERYCAP
VIDIOC_S_FMT YUYV 640×480
VIDIOC_S_PARM 30fps
VIDIOC_REQBUFS
VIDIOC_QUERYBUF
mmap
VIDIOC_QBUF
VIDIOC_STREAMON
poll/select 等待帧
VIDIOC_DQBUF
YUYV -> RGB888
VIDIOC_QBUF
VIDIOC_STREAMOFF
munmap
close
```

### 6.2 设计原则

```text
1. CameraAdapterV4L2 对上只输出 SensorFrame RGB888。
2. preprocess_node 不直接处理 YUYV。
3. readFrame 不无限阻塞，使用 poll/select 超时。
4. DQBUF 后必须 QBUF 归还 buffer。
5. stop() 必须释放 stream、mmap 和 fd。
6. 当前只实现 YUYV，不实现 MJPEG decode。
```

### 6.3 YUYV 到 RGB888

YUYV 数据排列：

```text
Y0 U0 Y1 V0
```

每 4 字节对应 2 个像素。转换后输出：

```text
RGBRGB
```

输出 `SensorFrame` 应满足：

```text
sensor_type = 0
format = RGB888
width = 640
height = 480
data_size = 921600
```

提交：

```bash
git add include/camera_adapter_v4l2.hpp src/camera_adapter_v4l2.cpp
git commit -m "v1.5-step2: implement V4L2 mmap YUYV camera adapter"
```

---

## 7. 阶段 E：新增 CameraAdapter example 11

### 7.1 新增文件

```text
examples/cpp/11_camera_adapter_v4l2.cpp
examples/Makefile
```

### 7.2 编译

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh
cd examples
make run EXAMPLE=11_camera_adapter_v4l2
```

若需传参：

```bash
cd ~/projects/autovision_mini_middleware_v1
./examples/bin/11_camera_adapter_v4l2 /dev/video0 640 480 30 30
```

### 7.3 验收标准

预期输出：

```text
[11_camera_adapter_v4l2] frame_id=1 sensor_type=0 width=640 height=480 data_size=921600
[11_camera_adapter_v4l2] frame_id=30 sensor_type=0 width=640 height=480 data_size=921600
[11_camera_adapter_v4l2] saved examples/logs/camera_first_frame.ppm
```

打开图片：

```bash
xdg-open examples/logs/camera_first_frame.ppm
```

判定：

```text
1. 能看到正常画面。
2. 颜色基本正确。
3. 没有紫色/绿色明显偏色。
4. data_size = 921600。
```

提交：

```bash
git add examples/cpp/11_camera_adapter_v4l2.cpp examples/Makefile
git commit -m "v1.5-step3: add V4L2 camera adapter example"
```

---

## 8. 阶段 F：接入 media_node camera 主链路

### 8.1 更新点

更新：

```text
src/media_node.cpp
scripts/run_camera_pipeline.sh
```

`media_node` 新增参数：

```text
--fps
```

构造 CameraAdapter：

```cpp
adapter = std::make_unique<CameraAdapterV4L2>(device, width, height, camera_fps);
```

当 `source=camera` 时，帧率由 V4L2 驱动控制，`media_node` 不再额外 sleep；当 `source=file` 或 `source=lidar_sim` 时继续保持原有 sleep 控制。

### 8.2 运行 camera pipeline

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh
./scripts/run_camera_pipeline.sh /dev/video0 120 640 480 30 dummy
cat logs/final_status_camera.txt
```

预期：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

检查日志：

```bash
tail -n 80 logs/media_camera.log
tail -n 80 logs/preprocess_camera.log
tail -n 80 logs/npu_camera.log
tail -n 80 logs/safety_camera.log
```

提交：

```bash
git add src/media_node.cpp scripts/run_camera_pipeline.sh
git commit -m "v1.5-step4: integrate V4L2 camera into media pipeline"
```

---

## 9. 阶段 G：重构 npu_stub_node 为 Runtime 抽象

### 9.1 新增 Runtime 接口

新增：

```text
include/runtime_config.hpp
include/runtime_output.hpp
include/inference_runtime.hpp
include/dummy_runtime.hpp
include/rknn_runtime_stub.hpp
include/runtime_factory.hpp
src/dummy_runtime.cpp
src/rknn_runtime_stub.cpp
src/runtime_factory.cpp
```

### 9.2 更新 npu_stub_node

更新：

```text
src/npu_stub_node.cpp
CMakeLists.txt
```

新结构：

```text
npu_stub_node
    -> open TensorPool
    -> open TensorMeta Ring
    -> pop TensorMeta
    -> read Tensor payload
    -> runtime->setInput(meta, data, size)
    -> runtime->run(output)
    -> write logs/npu_results.jsonl
```

支持参数：

```text
--backend dummy
--backend rknn_stub
--fake-latency 8
--model models/fake_model.tflite
```

### 9.3 独立验证文件输入链路不被破坏

```bash
./scripts/build.sh
./scripts/run_all_vm.sh
./build/control_client QUERY_STATUS
```

预期仍为：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

验证 rknn_stub：

```bash
./scripts/run_camera_pipeline.sh /dev/video0 60 640 480 30 rknn_stub
cat logs/final_status_camera.txt
```

提交：

```bash
git add CMakeLists.txt \
  include/runtime_config.hpp include/runtime_output.hpp include/inference_runtime.hpp \
  include/dummy_runtime.hpp include/rknn_runtime_stub.hpp include/runtime_factory.hpp \
  src/dummy_runtime.cpp src/rknn_runtime_stub.cpp src/runtime_factory.cpp src/npu_stub_node.cpp

git commit -m "v1.5-step5: refactor npu stub into SDK-style runtime abstraction"
```

---

## 10. 阶段 H：DMA-BUF 设计文档与性能报告

新增：

```text
docs/dmabuf_zero_copy_design.md
docs/performance_report_v1_5.md
```

当前仍不实现真实 DMA-BUF，因为 V1.5 目标是 Ubuntu/VMware 下的 V4L2 mmap + POSIX SHM 闭环。DMA-BUF 放在香橙派 5 Plus / RK3588 / MIPI / RKNN 阶段。

提交：

```bash
git add docs/dmabuf_zero_copy_design.md docs/performance_report_v1_5.md \
  docs/AutoVision_Mini_Middleware_V1_5_NextTasks_USB3_YUYV_Runtime_任务书.md

git commit -m "v1.5-step6: add dmabuf design and V1.5 next task documentation"
```

---

## 11. 阶段 I：最终验收矩阵

### 11.1 Camera 单元测试

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh
cd examples
make run EXAMPLE=11_camera_adapter_v4l2
```

通过标准：

```text
PPM 图片正常
data_size=921600
30 帧无 readFrame failed
```

### 11.2 文件输入回归

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/run_all_vm.sh
./build/control_client QUERY_STATUS
```

通过标准：

```text
media_frames=120
preprocess_frames=120
npu_frames=120
Safety=NORMAL
```

### 11.3 Camera pipeline

```bash
./scripts/run_camera_pipeline.sh /dev/video0 120 640 480 30 dummy
cat logs/final_status_camera.txt
```

通过标准：

```text
media_frames=120
preprocess_frames=120
npu_frames=120
crc_errors=0
frame_jumps=0
Safety=NORMAL
```

### 11.4 Runtime backend

```bash
./scripts/run_camera_pipeline.sh /dev/video0 60 640 480 30 rknn_stub
cat logs/final_status_camera.txt
```

通过标准：

```text
rknn_stub backend 可运行
不链接真实 RKNN SDK
JSON 输出包含 backend 字段
```

### 11.5 输出文件检查

```bash
ls -lh examples/logs/camera_first_frame.ppm
ls -lh logs/npu_results.jsonl
tail -n 5 logs/npu_results.jsonl
```

---

## 12. 最终 Git 合并流程

查看提交：

```bash
git log --oneline --decorate -n 10
```

推送分支：

```bash
git push -u origin v1.5-v4l2-runtime
```

如果你不想开 PR，直接合并到 main：

```bash
git checkout main
git pull
git merge --no-ff v1.5-v4l2-runtime -m "merge V1.5 V4L2 runtime pipeline"
git push
```

如果你想保留规范流程，先在 GitHub 上开 PR：

```text
base: main
compare: v1.5-v4l2-runtime
PR title: V1.5: Add V4L2 YUYV camera adapter and runtime abstraction
```

---

## 13. 本阶段完成后的简历/面试表述

```text
在 V1 文件输入闭环基础上，我进一步完成了 V1.5：基于 USB UVC 摄像头实现真实 V4L2 ioctl + mmap 采集，主格式为 YUYV 640×480，并在 CameraAdapter 内部完成 YUYV 到 RGB888 的转换，对上保持统一 SensorFrame 接口。随后将真实 Camera 输入接入 media_node、POSIX SHM FramePool、FrameMeta Ring、preprocess_node 和 Runtime 推理节点，形成 Camera -> SHM -> Preprocess -> Runtime -> Safety/Control 的端到端闭环。同时，我将原 npu_stub_node 重构为 SDK-style InferenceRuntime 抽象，支持 dummy 与 rknn_stub backend，为后续接入 RKNN/Pulsar2 等真实 NPU SDK 保留接口边界。
```

---

## 14. 下一版本 V2 建议

V1.5 完成后，V2 再推进：

```text
1. MJPEG fallback：V4L2 MJPG -> libjpeg-turbo -> RGB888。
2. 香橙派 5 Plus USB Camera 迁移：验证 ARM64 Linux 编译和运行。
3. OV13855 MIPI：设备树、驱动、media controller、ISP pipeline。
4. RKNN runtime：替换 rknn_stub。
5. DMA-BUF：VIDIOC_EXPBUF + fd passing + RGA/NPU import。
6. 引用计数：解决 frame_id % buffer_count 可能覆盖未消费 buffer 的问题。
```