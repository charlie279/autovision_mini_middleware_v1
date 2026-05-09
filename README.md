# AutoVision Mini Middleware V1

Ubuntu 22.04 / VMware 上的车载视觉中间件最小闭环 Demo。

本版本只依赖 C++17、CMake、POSIX SHM、pthread 和 Python3 生成测试输入，不依赖 OpenCV。目标是先跑通：

```text
FileAdapter / LidarSimAdapter
        -> media_node
        -> POSIX SHM FramePool
        -> FrameMeta Ring
        -> preprocess_node
        -> TensorMeta Ring
        -> npu_stub_node
        -> safety_monitor + control_service
```

## Quick Start

```bash
sudo apt update
sudo apt install -y build-essential cmake git python3 tree htop
chmod +x scripts/*.sh
./scripts/build.sh
./scripts/run_all_vm.sh
./build/control_client QUERY_STATUS
./scripts/collect_report.sh
```

## 异常注入

```bash
./scripts/inject_fault.sh bad_crc
./scripts/inject_fault.sh frame_jump
```

## V1 边界

- POSIX SHM 路径是 Ubuntu/x86_64 学习版，不是真正 DMA zero-copy。
- CameraAdapterV4L2 在 V1 中为占位实现；第二阶段开发板再实现真实 V4L2 ioctl + mmap。
- Safety Monitor 是 watchdog / E2E protection 学习版，不是 ISO 26262 / ASIL 级实现。
