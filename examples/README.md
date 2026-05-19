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

## V2.1 transport example

```bash
make run EXAMPLE=21_transport_four_links
```

This example validates the migrated reference transport-style RawFrame large-payload schema through the dependency-free local pub/sub backend. Expected result: `size_error=0`, `payload_error=0`, `result=PASS`.


## V2.4 camera FastDDS packet example

```bash
make run EXAMPLE=24_camera_fastdds_packet
```

This example validates 640x480 camera raw frame packet serialization without requiring FastDDS runtime. It checks that V2.4's 2MiB QoS accepts both YUYV and RGB888 raw camera payloads, while the old 512KiB boundary fails for a 640x480 YUYV frame as expected.

## V2.5 DDS + SHM frame metadata example

```bash
make run EXAMPLE=25_dds_shm_frame_meta
```

This example validates the V2.5 `SharedFrameDescriptor` codec without requiring FastDDS runtime. It writes a 640x480 YUYV payload into POSIX SHM, serializes only the descriptor metadata, parses it back, reads the payload from SHM, and verifies size/CRC. Expected result: `metadata_bytes=208`, `raw_payload_bytes=614400`, `size_errors=0`, `payload_errors=0`, `result=PASS`.

## V2.6 GStreamer pipeline config example

Example 26 is dependency-free. It validates that AutoVision can generate the intended GStreamer pipeline strings for:

```text
v4l2src -> capsfilter -> queue -> appsink name=appsink
appsrc name=appsrc -> videoconvert -> x264enc/x265enc/raw -> filesink
```

Run:

```bash
make run EXAMPLE=26_gstreamer_pipeline_config
```

Expected:

```text
[26_gstreamer_pipeline_config] raw_payload_bytes=614400 capture_ok=1 encode_ok=1 size_ok=1 result=PASS
```
