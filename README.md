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
