# AutoVision Mini Middleware V1.8 VM Performance Optimization

AutoVision Mini Middleware is a minimal C++17/Linux vision-middleware learning project for autonomous-driving middleware roles.

V1.8 consolidates V1.6 YUYV raw + fused preprocess and V1.7 benchmark visualization, then adds three VMware-friendly optimization points:

1. **Preprocess operator deep optimization**: RGB/YUYV resize index plan to remove per-pixel coordinate division from the hot loop.
2. **Frame buffer lifecycle simulation**: `FrameLeasePool` models acquire/publish/ref-count/release/timeout-reclaim semantics for later DMA-BUF/fd-passing work.
3. **Performance observability**: CSV/HTML benchmark logs, optional GStreamer camera baseline, optional `perf stat` profiling.

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

## Synthetic benchmark

```bash
./scripts/benchmark_v1_8_vm_perf_opt.sh 120
```

Generated outputs:

```text
logs/benchmark/v1_8_preprocess_compare_summary.csv
logs/benchmark/v1_8_preprocess_compare.html
logs/benchmark/v1_8_frame_lease_pool.csv
```

## Real USB camera verification on VMware Ubuntu

```bash
./scripts/check_usb_camera_v1_5.sh /dev/video0
./scripts/run_camera_pipeline.sh /dev/video0 300 640 480 30 dummy rgb
./scripts/run_camera_pipeline_yuyv_fused.sh /dev/video0 300 640 480 30 dummy
./scripts/benchmark_v1_7_camera_compare.sh /dev/video0 900 640 480 30 dummy
```

This repository still uses POSIX SHM + memcpy. It does not claim DMA-BUF zero-copy, real RKNN inference, or ISO 26262 compliance.
