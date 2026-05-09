# AutoVision Mini Middleware V1 设计说明

## 1. 设计定位

V1 只在 Ubuntu 22.04 / VMware 中验证软件闭环，不接真实摄像头、不接真实 NPU、不做 DMA-BUF。其目标是建立清晰的中间件数据链路：

```text
FileAdapter -> media_node -> POSIX SHM FramePool -> FrameMeta Ring
            -> preprocess_node -> TensorMeta Ring -> npu_stub_node
            -> shared_status -> safety_monitor / control_service
```

## 2. 工业对标边界

- `FramePool + FrameMeta Ring` 对标 EdgeFirst VideoStream 中“大数据放共享缓冲区、元数据跨进程传递”的思想。
- `preprocess_node` 对标 HAL 的 CPU fallback 前处理路径。
- `npu_stub_node` 对标 Model Service 的 `init/run/release` 接口边界。
- `control_service` 体现数据面和控制面分离。
- `safety_monitor` 体现 watchdog、CRC、frame_id 连续性和状态机监控。

## 3. V1 限制

V1 使用 POSIX SHM 和 memcpy，不是真正 zero-copy。第二阶段在开发板上再实现 V4L2 mmap、DMA-BUF、硬件编码和真实 NPU SDK。
