# AutoVision Mini Middleware V1.6：YUYV Raw SHM + 融合前处理性能优化任务书

> 版本定位：V1.6 是在 V1.5 已跑通真实 USB UVC / V4L2 mmap Camera pipeline 的基础上，新增的性能优化版本。V1.6 不重复建设 V1.5 已完成的 CameraAdapter、SHM、Runtime、Safety、Control 主链路，而是聚焦一个可量化亮点：把 CameraAdapter 默认 RGB888 中间帧路径扩展为可选 YUYV raw 输出，并在 preprocess_node 中融合完成 `YUYV -> RGB -> resize -> normalize`，降低帧 payload 带宽和无效整帧 RGB 转换开销。
>
> 当前补丁验证状态：本补丁已在 Linux 环境完成 CMake 编译、V1 文件输入链路回归和 `examples/12_yuyv_fused_preprocess` 合成 YUYV 前处理测试。由于当前运行环境没有 `/dev/video0` USB 摄像头，真实 Camera YUYV pipeline 需在你的 VMware Ubuntu 或香橙派 5 Plus 上执行实机验证。

---

## 1. V1.6 目标与边界

### 1.1 V1.6 目标

V1.6 只做一个明确优化点：

```text
V1.5 Camera RGB 中间帧路径：
USB UVC Camera /dev/video0
    -> V4L2 mmap YUYV
    -> CameraAdapterV4L2 内部 YUYV -> RGB888
    -> SensorFrame RGB888, 921600 bytes @ 640x480
    -> media_node memcpy to SHM
    -> preprocess_node RGB resize + normalize
    -> TensorPool

V1.6 YUYV raw + fused preprocess 路径：
USB UVC Camera /dev/video0
    -> V4L2 mmap YUYV
    -> CameraAdapterV4L2 输出 SensorFrame YUYV, 614400 bytes @ 640x480
    -> media_node memcpy to SHM
    -> preprocess_node fused YUYV -> RGB -> resize -> normalize
    -> TensorPool
```

V1.6 的主要验收目标：

```text
1. 保持 V1.5 RGB888 路径兼容，默认仍可走 --camera-output rgb。
2. 新增 --camera-output yuyv，CameraAdapterV4L2 输出 YUYV raw SensorFrame。
3. FrameMeta / SensorFrame 增加 stride_bytes 字段，为后续 DMA-BUF / RGA / MIPI CSI 预留工程接口。
4. preprocess_node 支持 RGB888 与 YUYV 两种输入格式。
5. 新增 image_preprocess.hpp/cpp，抽出可复用 CPU fallback 前处理函数。
6. 新增 examples/12_yuyv_fused_preprocess.cpp，不依赖真实摄像头即可验证融合前处理函数。
7. 新增 camera benchmark 脚本，用于在 VMware / 香橙派上对比 RGB 路径与 YUYV 融合路径。
```

### 1.2 V1.6 明确边界

```text
1. V1.6 仍不是 DMA-BUF zero-copy；仍然保留 SensorFrame vector 与 POSIX SHM memcpy。
2. V1.6 的优化重点是减少 CameraAdapter 输出 payload 大小和减少整帧 RGB 中间转换，不宣称实现硬件零拷贝。
3. V1.6 不接真实 RKNN Runtime；rknn_stub 仍只作为 SDK-style backend 占位。
4. V1.6 不接 MIPI CSI；香橙派阶段先继续使用 USB UVC 摄像头验证 ARM64 板端性能。
5. V1.6 不重构整体进程模型，不引入 DDS/SOME-IP/Zenoh。
```

---

## 2. V1.6 新增/修改文件清单

将补丁包中的以下文件按相同相对路径放入工程根目录并覆盖同名文件。

| 类型 | 路径 | 作用 |
|---|---|---|
| 修改 | `CMakeLists.txt` | 将 `src/image_preprocess.cpp` 加入 `avm_common` 编译。 |
| 修改 | `include/avm_config.hpp` | 新增 `kFormatYuyv = 'YUYV'` 像素格式常量。 |
| 修改 | `include/sensor_frame.hpp` | 新增 `stride_bytes` 字段。 |
| 修改 | `include/frame_meta.hpp` | 新增 `stride_bytes` 字段。 |
| 修改 | `include/camera_adapter_v4l2.hpp` | 新增 `CameraOutputFormat`，支持 RGB888 / YUYV 输出选择。 |
| 修改 | `src/camera_adapter_v4l2.cpp` | V4L2 mmap YUYV 采集后可选择输出 RGB888 或 YUYV compact payload。 |
| 修改 | `src/media_node.cpp` | 新增 `--camera-output rgb|yuyv` 参数，并向 FrameMeta 传递 stride。 |
| 修改 | `src/preprocess_node.cpp` | 支持 RGB888 和 YUYV 输入，调用融合前处理函数。 |
| 新增 | `include/image_preprocess.hpp` | 前处理函数声明。 |
| 新增 | `src/image_preprocess.cpp` | RGB888 resize/normalize 与 YUYV 融合前处理实现。 |
| 修改 | `examples/Makefile` | 新增 example 12 编译入口。 |
| 新增 | `examples/cpp/12_yuyv_fused_preprocess.cpp` | 合成 YUYV 数据，验证融合前处理函数，不依赖摄像头。 |
| 修改 | `scripts/run_camera_pipeline.sh` | 增加第 7 个参数 `camera_output`。 |
| 新增 | `scripts/run_camera_pipeline_yuyv_fused.sh` | YUYV fused pipeline 快捷启动脚本。 |
| 新增 | `scripts/benchmark_v1_6_camera_compare.sh` | RGB vs YUYV 两条 Camera 路径性能对比脚本。 |
| 新增 | `docs/AutoVision_Mini_Middleware_V1_6_NextTasks_YUYV_Fused_Preprocess_任务书.md` | 本任务书。 |

---

## 3. V1.6 模块设计

### 3.1 CameraAdapterV4L2 输出模式

新增枚举：

```cpp
    enum class CameraOutputFormat : std::uint32_t {
        RGB888 = 0,
        YUYV = 1
    };
```

CameraAdapter 构造函数增加 `output_format` 参数：

```cpp
CameraAdapterV4L2(device, width, height, fps, CameraOutputFormat::YUYV)
```

运行时由 `media_node` 通过命令行参数选择：

```bash
./build/media_node --source camera --device /dev/video0 --width 640 --height 480 --fps 30 --frames 120 --camera-output rgb
./build/media_node --source camera --device /dev/video0 --width 640 --height 480 --fps 30 --frames 120 --camera-output yuyv
```

### 3.2 数据结构更新

`SensorFrame` 和 `FrameMeta` 新增：

```cpp
std::uint32_t stride_bytes = 0;
```

原因：

```text
1. V4L2 / MIPI / ISP 输出的 bytes_per_line 不一定等于 width * bytes_per_pixel。
2. 后续 DMA-BUF / RGA / NPU import 都需要 width、height、format、stride 同时传递。
3. 当前 V1.6 先用 compact YUYV payload，stride = width * 2；RGB888 stride = width * 3。
```

### 3.3 image_preprocess 抽象

新增 `image_preprocess.hpp/cpp`，把前处理从 `preprocess_node.cpp` 中抽离成可复用函数：

```cpp
void resize_rgb888_to_tensor(...);
void resize_yuyv_to_tensor(...);
```

其中 `resize_yuyv_to_tensor()` 直接完成：

```text
输出 Tensor 像素坐标
    -> 反算源图 src_x / src_y
    -> 读取 YUYV pair 中的 Y0/Y1/U/V
    -> YUV 转 RGB
    -> normalize 到 0~1 float
    -> 写入 320x320x3 Tensor
```

该路径避免先生成完整 640x480x3 RGB888 中间帧。

### 3.4 带宽收益

以你当前 USB 摄像头主路径 `640x480@30fps` 为例：

| 路径 | 每帧 payload | 30fps payload 带宽 |
|---|---:|---:|
| V1.5 RGB888 中间帧 | 640×480×3 = 921,600 B | 27.65 MB/s |
| V1.6 YUYV raw | 640×480×2 = 614,400 B | 18.43 MB/s |
| 理论节省 | -307,200 B/frame | -9.22 MB/s，约 -33.3% |

该收益是确定的工程账；实际 CPU 占用与 preprocess P95 延迟需要在 VMware / 香橙派实机 benchmark 后填写。

---

## 4. 编译与基础验证

### 4.1 应用补丁

假设你的工程位于：

```bash
cd ~/projects/autovision_mini_middleware_v1
```

解压补丁包后，将 `patch_files/` 下的文件复制到工程根目录：

```bash
cp -r patch_files/* ~/projects/autovision_mini_middleware_v1/
cd ~/projects/autovision_mini_middleware_v1
chmod +x scripts/*.sh
```

### 4.2 编译

```bash
./scripts/build.sh
```

预期：

```text
[100%] Built target media_node
[100%] Built target preprocess_node
[100%] Built target npu_stub_node
[build] done
```

### 4.3 V1 文件输入链路回归

V1.6 必须保证 V1/V1.5 文件输入链路不被破坏：

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

预期：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

### 4.4 合成 YUYV 融合前处理测试

该测试不依赖 `/dev/video0`，用于验证 V1.6 新增前处理函数能在普通 Linux 环境编译运行：

```bash
cd examples
make clean || true
make all
make run EXAMPLE=12_yuyv_fused_preprocess
```

预期输出类似：

```text
[12_yuyv_fused_preprocess] input_format=YUYV input_bytes=614400 output_tensor_bytes=1228800 mean=...ms p95=...ms
```

---

## 5. VMware Ubuntu 实机 Camera 验证

### 5.1 保持 V1.5 RGB 路径回归

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/clean_ipc.sh
./scripts/run_camera_pipeline.sh /dev/video0 120 640 480 30 dummy rgb
cat logs/final_status_camera.txt
```

预期：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

### 5.2 验证 V1.6 YUYV fused 路径

```bash
./scripts/clean_ipc.sh
./scripts/run_camera_pipeline_yuyv_fused.sh /dev/video0 120 640 480 30 dummy
cat logs/final_status_camera.txt
```

或直接运行：

```bash
./scripts/run_camera_pipeline.sh /dev/video0 120 640 480 30 dummy yuyv
```

关键日志应包含：

```text
[CameraAdapterV4L2] frame_id=... output_format=YUYV payload_size=614400
[Preprocess] frame_id=... input_format=YUYV preprocess=...ms total=...ms
[preprocess_node] finished processed=120 crc_errors=0 frame_jumps=0 unsupported_format=0
```

---

## 6. V1.6 性能对比任务

### 6.1 一键对比脚本

```bash
./scripts/benchmark_v1_6_camera_compare.sh /dev/video0 900 640 480 30 dummy
```

该脚本会依次运行：

```text
rgb  路径：V1.5-compatible RGB888 intermediate path
yuyv 路径：V1.6 YUYV raw + fused preprocess path
```

结果保存到：

```text
logs/benchmark_v1_6/rgb/
logs/benchmark_v1_6/yuyv/
```

### 6.2 需要记录的指标

| 指标 | RGB 路径 | YUYV fused 路径 | 说明 |
|---|---:|---:|---|
| input payload/frame | 921600 B | 614400 B | 理论固定值 |
| input payload bandwidth @30fps | 27.65 MB/s | 18.43 MB/s | 理论固定值 |
| preprocess mean | 实测填写 | 实测填写 | 从 preprocess log 读取 |
| preprocess p95 | 实测填写 | 实测填写 | 从 preprocess log 读取 |
| total mean | 实测填写 | 实测填写 | 从 preprocess log 读取 |
| total p95 | 实测填写 | 实测填写 | 从 preprocess log 读取 |
| media/preprocess/npu frames | 实测填写 | 实测填写 | 应三者一致 |
| crc_errors | 实测填写 | 实测填写 | 预期为 0 |
| frame_jumps | 实测填写 | 实测填写 | 预期为 0 |
| unsupported_format | 实测填写 | 实测填写 | 预期为 0 |

---

## 7. 香橙派 5 Plus 迁移任务

### 7.1 板端第一阶段：不接 MIPI，先接 USB 摄像头

```bash
sudo apt update
sudo apt install -y build-essential cmake git v4l-utils ffmpeg usbutils htop

git clone https://github.com/charlie279/autovision_mini_middleware_v1.git
cd autovision_mini_middleware_v1
git checkout v1.6-yuyv-fused-preprocess
chmod +x scripts/*.sh
./scripts/build.sh
```

检查摄像头：

```bash
ls -l /dev/video*
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 --list-formats-ext
```

先验证 YUYV 640x480@30fps：

```bash
./scripts/check_usb_camera_v1_5.sh /dev/video0
```

再运行 V1.6：

```bash
./scripts/run_camera_pipeline_yuyv_fused.sh /dev/video0 300 640 480 30 dummy
cat logs/final_status_camera.txt
```

### 7.2 板端第二阶段：采集性能报告

```bash
./scripts/benchmark_v1_6_camera_compare.sh /dev/video0 900 640 480 30 dummy
```

同时另开终端观察：

```bash
htop
vmstat 1
```

记录 CPU、内存、温度和日志指标。

### 7.3 板端第三阶段：为 RGA / DMA-BUF 预留接口

V1.6 已新增 `format + stride_bytes`，后续 V1.7/V2 可继续演进：

```text
V1.7: RGA color convert / resize fallback
V2.0: DMA-BUF fd passing + buffer lifecycle
V2.1: RKNN Runtime real backend
```

---

## 8. 分阶段 Git 指令

### 阶段 0：创建 V1.6 分支

```bash
cd ~/projects/autovision_mini_middleware_v1
git checkout v1.5-v4l2-runtime
git pull
git checkout -b v1.6-yuyv-fused-preprocess
```

### 阶段 1：提交数据结构与格式常量

```bash
git add include/avm_config.hpp include/sensor_frame.hpp include/frame_meta.hpp
git commit -m "v1.6-step1: add YUYV format and stride metadata"
```

### 阶段 2：提交 CameraAdapter YUYV 输出模式

```bash
git add include/camera_adapter_v4l2.hpp src/camera_adapter_v4l2.cpp src/media_node.cpp
git commit -m "v1.6-step2: support YUYV camera output mode"
```

### 阶段 3：提交融合前处理模块

```bash
git add include/image_preprocess.hpp src/image_preprocess.cpp src/preprocess_node.cpp CMakeLists.txt
git commit -m "v1.6-step3: add fused YUYV preprocess path"
```

### 阶段 4：提交 examples 测试

```bash
git add examples/Makefile examples/cpp/12_yuyv_fused_preprocess.cpp
git commit -m "v1.6-step4: add YUYV fused preprocess example"
```

### 阶段 5：提交运行和 benchmark 脚本

```bash
git add scripts/run_camera_pipeline.sh scripts/run_camera_pipeline_yuyv_fused.sh scripts/benchmark_v1_6_camera_compare.sh
git commit -m "v1.6-step5: add camera YUYV pipeline benchmark scripts"
```

### 阶段 6：提交任务书

```bash
git add docs/AutoVision_Mini_Middleware_V1_6_NextTasks_YUYV_Fused_Preprocess_任务书.md
git commit -m "v1.6-step6: add YUYV fused preprocess task document"
```

### 阶段 7：最终推送

```bash
git log --oneline --decorate -n 10
git push -u origin v1.6-yuyv-fused-preprocess
```

可选创建 PR：

```text
base: v1.5-v4l2-runtime 或 main
compare: v1.6-yuyv-fused-preprocess
PR title: V1.6: Add YUYV raw camera path and fused preprocess optimization
```

---

## 9. 面试表达主线

可以这样讲：

```text
V1.5 我已经跑通真实 USB UVC 摄像头到 SHM、前处理、Runtime Stub 和 Safety Monitor 的完整链路。V1.6 我没有继续盲目堆功能，而是做了一个可量化的性能优化：原来 CameraAdapter 在采集到 YUYV 后会先转换成 640x480 RGB888，再写入共享内存；V1.6 支持直接输出 YUYV raw frame，并在 preprocess_node 中融合完成 YUYV 到 RGB、resize 和 normalize。

这样 640x480 单帧 payload 从 921600 bytes 降到 614400 bytes，30fps 下输入 payload 带宽从 27.65MB/s 降到 18.43MB/s，理论上减少约 33.3%。同时我把 format 和 stride 加入 SensorFrame/FrameMeta，为后续在 RK3588 上接 RGA 或 DMA-BUF 预留了接口。这个优化体现的是中间件视角：关注数据搬运、格式边界、模块职责和可量化性能指标，而不是只把算法跑起来。
```

---

## 10. V1.6 完成标准

V1.6 合格标准：

```text
1. ./scripts/build.sh 编译通过。
2. ./scripts/run_all_vm.sh 文件输入回归 NORMAL。
3. examples/12_yuyv_fused_preprocess 编译运行通过。
4. VMware 上 /dev/video0 跑通 rgb 与 yuyv 两条 Camera pipeline。
5. yuyv 路径日志显示 payload_size=614400，preprocess input_format=YUYV。
6. benchmark_v1_6_camera_compare.sh 生成 rgb/yuyv 两组日志。
7. docs 中补充一张 V1.5 RGB vs V1.6 YUYV 的性能对比表。
```

达到以上标准后，下一阶段再做香橙派 5 Plus 板端迁移；不要同时引入 MIPI、RKNN、RGA、DDS/SOME-IP，避免任务失控。
