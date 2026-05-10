# AutoVision Mini Middleware V1 Examples

本目录用于通过 **独立 C++ 示例程序** 理解主工程各模块的接口调用方式。它不是替代 `scripts/run_all_vm.sh` 的工程级验收脚本，而是用于学习每个类、结构和 IPC 原语的最小调用路径。

## 目录结构

```text
examples/
├── Makefile
├── README.md
├── cpp/
│   ├── 00_crc32.cpp
│   ├── 01_latency_profiler.cpp
│   ├── 02_control_cmd.cpp
│   ├── 03_file_adapter.cpp
│   ├── 04_lidar_sim_adapter.cpp
│   ├── 05_shm_frame_pool.cpp
│   ├── 06_shm_ring_buffer.cpp
│   ├── 07_shared_status.cpp
│   ├── 08_frame_producer.cpp
│   ├── 08_frame_consumer.cpp
│   ├── 09_sensor_to_shm.cpp
│   └── 10_control_query.cpp
├── bin/      # make 自动生成，不提交
└── logs/     # 运行日志，不提交
```

## 使用前准备

在工程根目录执行：

```bash
./scripts/build.sh
./scripts/prepare_input.sh
```

进入 examples：

```bash
cd examples
make list
```

## 单个示例运行

```bash
make run EXAMPLE=00_crc32
make run EXAMPLE=03_file_adapter
make run EXAMPLE=05_shm_frame_pool
make run EXAMPLE=06_shm_ring_buffer
```

## 编译全部示例

```bash
make all
```

## 双进程 IPC 示例

```bash
make run_frame_ipc
```

该示例会启动 `08_frame_consumer` 等待 FrameMeta，再运行 `08_frame_producer` 写入 FramePool 和 Ring，模拟主工程中的数据面：

```text
producer → ShmFramePool(payload) + ShmRingBuffer(FrameMeta) → consumer
```

## 控制面查询示例

先在工程根目录启动控制服务：

```bash
./build/control_service
```

另开终端：

```bash
cd examples
make run EXAMPLE=10_control_query
```

## 与 scripts 的区别

```text
examples/   关注 C++ 接口调用，适合理解模块边界
scripts/    关注工程级验收，适合跑端到端链路
src/        主工程多进程节点
include/    模块 API 和数据结构
```
