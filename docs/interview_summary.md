# 面试讲解稿

这个项目是一个车载视觉中间件最小闭环 Demo。第一版先在 Ubuntu 22.04 中验证软件链路，避免一开始被开发板驱动、摄像头和 NPU SDK 阻塞。

架构上，我把输入抽象为 SensorAdapter，当前支持 FileAdapter 和 LidarSimAdapter；media_node 把帧 payload 写入 POSIX SHM FramePool，跨进程只通过 Ring 传 FrameMeta；preprocess_node 完成 resize 和 normalize，并输出 TensorMeta；npu_stub_node 模拟 NPU SDK 的 init/run/release 边界；control_service 通过 Unix Domain Socket 提供控制面；safety_monitor 通过共享状态区检查 heartbeat、CRC、frame_id 连续性和 timeout。

这个项目证明的不是算法精度，而是我对自动驾驶中间件核心问题的理解：传感器数据如何抽象，图像帧如何低拷贝传输，前处理和 NPU 如何解耦，控制面和数据面如何分离，以及链路如何做运行时监控。