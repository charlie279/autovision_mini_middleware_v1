# AutoVision Mini Middleware V1.8 Linux Validation Report

## Environment

- Host: ChatGPT Linux sandbox container
- Camera: not available (`/dev/video0` not present)
- Compiler: system g++ with C++17
- Build system: CMake + Make

## Verified

1. `./scripts/build.sh`: passed.
2. `./scripts/prepare_input.sh`: generated/verified `assets/input_640x480_rgb.raw`.
3. `./scripts/run_all_vm.sh`: passed file-input end-to-end regression.
4. `cat logs/final_status.txt`: `status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"`.
5. `cd examples && make all`: passed all examples 00~14 compile.
6. `make run EXAMPLE=12_yuyv_fused_preprocess`: passed synthetic YUYV fused preprocess test.
7. `./scripts/benchmark_v1_8_vm_perf_opt.sh 80`: passed preprocess benchmark + frame lease validation and generated CSV/HTML logs.

## Benchmark snapshot from this Linux sandbox

The benchmark is synthetic and only proves the code path can run and produce comparable logs. Do not treat the absolute latency as USB camera or Orange Pi 5 Plus results.

```text
rgb_input_bytes=921600 div_mean_ms≈0.40 plan_mean_ms≈0.38
yuyv_input_bytes=614400 div_mean_ms≈0.93 plan_mean_ms≈0.69
```

## Waiting for user-side verification

1. VMware Ubuntu + real `/dev/video0` USB UVC camera:
   - `./scripts/check_usb_camera_v1_5.sh /dev/video0`
   - `./scripts/run_camera_pipeline.sh /dev/video0 300 640 480 30 dummy rgb`
   - `./scripts/run_camera_pipeline_yuyv_fused.sh /dev/video0 300 640 480 30 dummy`
   - `./scripts/benchmark_v1_7_camera_compare.sh /dev/video0 900 640 480 30 dummy`
2. Optional GStreamer camera baseline:
   - `./scripts/benchmark_gstreamer_camera.sh /dev/video0 300 640 480 30`
3. Optional perf profiling:
   - `./scripts/profile_preprocess_perf.sh 1000`
4. Orange Pi 5 Plus ARM64 migration:
   - compile, file-input regression, USB camera RGB/YUYV benchmark, CPU/temperature monitoring.
