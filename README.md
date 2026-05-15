# AutoVision Mini Middleware V1.9 Feature Pack

AutoVision Mini Middleware is a minimal Linux/C++17 vision-middleware project for autonomous-driving middleware roles.

This package is based on the V1.8 `v1.8-vm-visual-perf-framelease` code line and adds the next middleware-oriented functions:

1. `UdpLidarSimAdapter` and `TimestampSync` for UDP-based simulated lidar input and approximate camera/lidar sync.
2. Ring queue `depth/drop_count`, `drop_oldest` policy, and binary `record/replay`.
3. Unix domain socket `SCM_RIGHTS` fd passing example plus `alive_counter` validation.
4. Safety monitor alive-counter detection and automatic degraded FPS policy.
5. NV12 preprocess and dependency-free `libyuv_compat` backend entry.
6. `RuntimeMetadata` and RKNN-like IO query mock.
7. `DetectionResult` schema and `algorithm_stub_node`.
8. RAII fd/mmap utilities and CPU affinity/priority benchmark.

## Build

```bash
chmod +x scripts/*.sh
./scripts/build.sh
```

## File-input regression

```bash
./scripts/prepare_input.sh
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

Expected:

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## New feature validation

```bash
cd examples
make all
make run EXAMPLE=15_fd_passing_alive_counter
make run EXAMPLE=16_udp_lidar_timestamp_sync
make run EXAMPLE=17_nv12_libyuv_preprocess
make run EXAMPLE=18_runtime_metadata_detection
make run EXAMPLE=19_affinity_priority_benchmark
```

One-shot validation:

```bash
./scripts/benchmark_v1_9_featurepack.sh 10
```

Expected summary:

```text
main_build=PASS
file_pipeline=PASS
algorithm_stub=PASS
examples_15_19=PASS
record_replay=PASS
affinity_priority=PASS_WITH_NON_ROOT_PRIORITY_FALLBACK
real_camera=WAIT_USER_VMWARE_OR_BOARD
```

## Camera verification on VMware Ubuntu

```bash
./scripts/check_usb_camera_v1_5.sh /dev/video0
./scripts/run_camera_pipeline.sh /dev/video0 300 640 480 30 dummy rgb
./scripts/run_camera_pipeline_yuyv_fused.sh /dev/video0 300 640 480 30 dummy
```

This repository still uses POSIX SHM + memcpy. It does not claim production DMA-BUF zero-copy, real RKNN inference, real lidar driver, or ISO 26262 compliance.
