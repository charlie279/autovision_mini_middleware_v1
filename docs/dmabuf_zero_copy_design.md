# DMA-BUF / Zero-Copy Design for AutoVision Mini Middleware

## 1. 当前 V1/V1.5 数据路径

V1/V1.5 在 Ubuntu / VMware 中使用 POSIX SHM + memcpy。它是学习版低拷贝 IPC，不是真正 zero-copy。

```text
USB UVC Camera
    -> V4L2 mmap buffer
    -> CPU YUYV to RGB888
    -> SensorFrame.data
    -> media_node memcpy to POSIX SHM FramePool
    -> FrameMeta Ring(buffer_index)
    -> preprocess_node read payload
    -> CPU resize + normalize
    -> TensorPool
    -> TensorMeta Ring
    -> Runtime backend
```

这个路径的优点是可移植、容易调试、能在 x86 Ubuntu 上运行；缺点是 CameraAdapter、FramePool、Preprocess 之间仍有 CPU 拷贝。

## 2. 为什么当前不是 zero-copy

严格意义的 zero-copy 需要 Camera/ISP/硬件前处理/NPU 之间共享同一块 DMA-capable buffer。当前路径中至少存在：

```text
V4L2 mmap buffer -> RGB vector
RGB vector -> POSIX SHM FramePool
FramePool -> preprocess raw vector
preprocess tensor vector -> TensorPool
```

因此当前版本只能称为“元数据低拷贝 + payload 共享内存”，不能称为 DMA zero-copy。

## 3. 目标 DMA-BUF 数据路径

V2/V3 在开发板上应演进为：

```text
V4L2 driver alloc buffer
    -> VIDIOC_EXPBUF export dma_buf_fd
    -> FrameMetaV2 carries fd / format / stride / width / height
    -> preprocess_node imports dma_buf_fd
    -> RGA/G2D/OpenCL/NPU preprocess
    -> NPU runtime imports tensor buffer
    -> result metadata only
```

## 4. FrameMetaV2 建议

```cpp
struct FrameMetaV2 {
    std::uint64_t frame_id;
    std::uint64_t timestamp_ns;
    std::uint32_t sensor_type;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t format;
    std::uint32_t stride;
    std::uint32_t data_size;
    std::int32_t dma_buf_fd;
    std::uint32_t buffer_index;
    std::uint32_t crc32;
    std::uint32_t alive_counter;
    std::uint32_t status;
};
```

## 5. 生命周期与同步

DMA-BUF 版本必须补充：

```text
1. buffer owner：谁分配，谁释放。
2. fd passing：跨进程传递 fd 不能只靠普通 SHM，需要 Unix Domain Socket SCM_RIGHTS 或同进程内共享 handle。
3. 引用计数：消费者未完成前，生产者不能复用该 buffer。
4. cache sync：CPU 访问和设备 DMA 访问之间需要同步。
5. fence/sync_file：硬件模块之间需要完成信号。
6. error path：消费者异常退出时，buffer 不能永久泄漏。
```

## 6. 开发阶段划分

```text
V1.5-Desktop:
    保持 POSIX SHM + memcpy。
    只写 DMA-BUF 设计文档，不实现真实 fd passing。

V2-Board-Basic:
    在香橙派 5 Plus / RK3588 上用 USB Camera 复用 V4L2 YUYV 路径。
    验证 ARM64 Linux 编译、运行、CPU 占用。

V3-Board-MIPI/NPU:
    接 MIPI Camera / RKNN / RGA。
    根据驱动支持情况验证 VIDIOC_EXPBUF / dma_buf_fd import。
```
