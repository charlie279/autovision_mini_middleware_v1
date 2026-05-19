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


## V2.1 reference transport Transport Bridge

V2.1 adds a dependency-free reference transport-style transport bridge inside AutoVision. It does not pull FastDDS into the AutoVision build. Instead, it migrates the useful engineering part from reference transport v4.7.2: message schema, QoS payload guard, bounded queue depth/drop statistics, and four-link large-payload validation for `TestMsg`, `RawFrame`, `EncodedFrame`, and `LidarFrame`.

Build and run the four-link validation:

```bash
./scripts/build.sh
./scripts/benchmark_transport_four_links.sh 30
cat logs/benchmark_v2_1_transport/four_links.csv
```

Run a single mode:

```bash
./build/transport_four_links_demo --mode raw --frames 30 --output logs/raw_transport.csv
```

Example-level test:

```bash
cd examples
make run EXAMPLE=21_transport_four_links
```

Scope boundary: V2.1 is a local pub/sub and payload-safety validation backend. It is not a FastDDS runtime inside AutoVision, and it does not claim ROS2/DDS network discovery or RTPS interoperability. The next step is to introduce an `ITransportBackend` adapter for optional FastDDS / SHM / RTOS backends.


## V2.2 Transport Pattern

V2.2 adds a reference transport-style communication abstraction that matches the Transmitter / Dispatcher / Receiver design pattern:

```text
Transport::createTransmitter() -> Transmitter -> RtpsTransmitter / ShmTransmitter
Transport::createReceiver()    -> Receiver    -> RtpsReceiver / ShmReceiver
Dispatcher layer               -> RtpsDispatcher / ShmDispatcher
```

Run:

```bash
./scripts/benchmark_transport_pattern.sh 5 8
```

Notes:

- `rtps` is currently a dependency-free RTPS-style local backend, not a real FastDDS endpoint.
- `shm` is currently a dependency-free local SHM-style backend for abstraction and payload validation.
- Both backends validate reference transport-style Test / RawFrame / EncodedFrame / LidarFrame payloads, sequence, CRC, lost/drop counters and message-size boundaries.

## V2.3 FastDDS/RTPS adapter

V2.3 adds an AutoVision-owned optional FastDDS/Fast-RTPS adapter. The default build remains dependency-free and keeps the V2.2 local RTPS-style and SHM-style validation paths. When FastDDS/Fast-RTPS 2.12.x and Fast-CDR are installed, rebuild with:

```bash
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=/path/to/fastdds/install
```

Default dependency-free validation:

```bash
./scripts/build.sh
./scripts/benchmark_fastdds_rtps.sh 5 8
cd examples && make run EXAMPLE=23_fastdds_packet_codec
```

The optional FastDDS adapter uses AutoVision `TransportEnvelope` serialization and keeps third-party implementation details outside the public project namespace.


## V2.4 Camera FastDDS Pub/Sub

V2.4 adds a two-process Camera FastDDS validation path on top of the V2.3 optional FastDDS/RTPS adapter:

```text
USB Camera /dev/video0 or synthetic source
    -> camera_fastdds_pub
    -> FastDDS / RTPS topic
    -> camera_fastdds_sub
    -> size / sequence / CRC / latency validation
```

The default build remains dependency-free. Without FastDDS/Fast-RTPS installed, the new programs report `NOT_COMPILED` explicitly.

Dependency-free validation:

```bash
./scripts/build.sh
./scripts/benchmark_camera_fastdds_pub_sub.sh 5 640 480 30 yuyv
cd examples && make run EXAMPLE=24_camera_fastdds_packet
```

Real FastDDS/Fast-RTPS validation after installing FastDDS/Fast-CDR:

```bash
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
./scripts/benchmark_camera_fastdds_pub_sub.sh 60 640 480 30 yuyv avm/camera/synthetic_raw
./scripts/run_camera_fastdds_pub_sub.sh /dev/video0 300 640 480 30 yuyv avm/camera/raw
```

V2.4 increases the default FastDDS adapter payload limit to 2MiB so that 640x480 YUYV raw frames (614400 bytes) and RGB888 raw frames (921600 bytes) can pass the transport payload guard.

## V2.4 Camera FastDDS Pub/Sub stable validation revision

V2.4 adds two-process Camera FastDDS publisher/subscriber demos:

```bash
./scripts/benchmark_camera_fastdds_pub_sub.sh 60 640 480 30 yuyv avm/camera/synthetic_raw
./scripts/run_camera_fastdds_pub_sub.sh /dev/video0 60 640 480 30 yuyv avm/camera/raw
```

The stable-validation revision uses:

- `--startup-ms 5000` before the first publish, giving DDS endpoint discovery time to match.
- `--warmup-frames 10` and subscriber `--start-seq 11`, so warmup traffic is excluded from formal validation.
- `--repeat 10` plus subscriber-side de-duplication, improving robustness for RTPS/UDP fragmented raw camera frames in VMware.
- `--depth 1024`, `--timeout-ms 120000` and `--reliable` by default in V2.4 scripts.

Default builds without FastDDS still compile and report `NOT_COMPILED` clearly. Real FastDDS validation requires rebuilding with `-DAVM_ENABLE_FASTDDS=ON` and a valid FastDDS/Fast-CDR installation.

## V2.5 DDS + SHM mixed route

V2.5 adds a mixed-route Camera middleware validation path on top of V2.4:

```text
USB Camera / synthetic source
    -> POSIX SHM FramePool raw payload
    -> FastDDS / RTPS SharedFrameDescriptor metadata
    -> subscriber reads SHM payload
    -> size / CRC / sequence / latency validation
```

Compared with V2.4 raw frame over FastDDS, V2.5 keeps DDS payload small by publishing only metadata. For 640x480 YUYV, the raw payload is 614400 bytes while the descriptor payload is about 208 bytes.

Dependency-free validation:

```bash
./scripts/build.sh
cd examples && make run EXAMPLE=25_dds_shm_frame_meta
./scripts/benchmark_camera_dds_shm.sh 5 640 480 30 yuyv
```

Real FastDDS validation after installing FastDDS/Fast-CDR:

```bash
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
./scripts/benchmark_camera_dds_shm.sh 300 640 480 30 yuyv avm/camera/dds_shm_synthetic_meta /avm_camera_dds_shm_synthetic_pool
./scripts/run_camera_dds_shm.sh /dev/video0 300 640 480 30 yuyv avm/camera/dds_shm_raw_meta /avm_camera_dds_shm_raw_pool
```

Scope boundary: V2.5 validates metadata over DDS plus payload over POSIX SHM on a single machine. It does not claim cross-host SHM sharing, DMA-BUF zero-copy, typed DDS IDL, or production functional safety certification.

## V2.6 GStreamer Camera Pipeline

V2.6 adds an optional GStreamer media-framework path on top of the V2.5 DDS + SHM mixed-route base:

```text
USB Camera /dev/video0
    -> v4l2src
    -> video/x-raw capsfilter
    -> queue
    -> appsink name=appsink
    -> GStreamerCameraAdapter
    -> AutoVision SensorFrame
```

It also adds an appsrc-based encode/stream demo:

```text
SensorFrame synthetic or GStreamer camera frame
    -> appsrc name=appsrc
    -> videoconvert
    -> x264enc / x265enc / raw filesink
```

Default builds remain dependency-free and the new binaries report `NOT_COMPILED` when GStreamer headers/libraries are not enabled. Real GStreamer builds require:

```bash
./scripts/build.sh -DAVM_ENABLE_GSTREAMER=ON
```

Dependency-free validation:

```bash
./scripts/build.sh
cd examples && make run EXAMPLE=26_gstreamer_pipeline_config
./scripts/benchmark_gstreamer_camera.sh /dev/video0 5 640 480 30 yuyv
```

Real camera validation after installing GStreamer development/runtime packages:

```bash
./scripts/build.sh -DAVM_ENABLE_GSTREAMER=ON
./scripts/run_gstreamer_camera_capture.sh /dev/video0 300 640 480 30 yuyv
./scripts/run_gstreamer_encode_stream.sh synthetic /dev/video0 120 640 480 30 yuyv h264 logs/gstreamer_output.h264
```

Scope boundary: V2.6 validates GStreamer appsink/appsrc integration. The `--use-dmabuf` path requests `v4l2src io-mode=dmabuf`, but the current appsink adapter still maps/copies payload into `SensorFrame.data`; true DMA-BUF fd handoff and zero-copy ownership transfer are reserved for board-side V3.x work.
