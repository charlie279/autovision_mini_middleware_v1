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

## V2.0 Video Encode

V2.0 adds an independent `encode_sink_node` process and a `VideoEncoder` factory layer.

Build:

```bash
./scripts/build.sh
```

Run H.264 encode pipeline:

```bash
./scripts/run_encode_pipeline.sh soft h264 30
```

Run H.265 encode pipeline:

```bash
./scripts/run_encode_pipeline.sh soft h265 10
```

Run encode benchmark:

```bash
./scripts/benchmark_encode.sh 30 640 480
```

Notes:

- `soft` uses `ffmpeg` CLI to produce real H.264/H.265 elementary streams when the command is available.
- `mpp` and `v4l2m2m` are compile-safe backend stubs in generic Linux/VMware; replace their internals on RK3588/OPi5+ or a board with V4L2 M2M codec devices.
