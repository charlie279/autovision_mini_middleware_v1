# EdgeFirstAI 对标映射

| 本项目文件 | 对标方向 | 说明 |
|---|---|---|
| `media_node.cpp` | Camera / VideoStream producer | 负责产生帧并发布元数据 |
| `shm_frame_pool.cpp` | POSIX SHM fallback | V1 用共享内存模拟大帧传输 |
| `shm_ring_buffer.cpp` | Metadata IPC | Ring 只传 FrameMeta / TensorMeta |
| `preprocess_node.cpp` | HAL CPU fallback | resize + normalize |
| `npu_stub_node.cpp` | Model Service | 模拟 NPU SDK 接口 |
| `control_service.cpp` | Control Plane | 控制面独立于图像数据面 |
| `safety_monitor.cpp` | Watchdog / E2E protection | heartbeat、CRC、frame jump、timeout |
