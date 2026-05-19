# V2.6 GStreamer Camera Pipeline Container Validation

## Environment

```text
Date: 2026-05-19
Container: Linux sandbox
Compiler: GNU C++ 14.2.0
GStreamer dev packages: not installed for pkg-config gstreamer-1.0 / gstreamer-app-1.0
FastDDS/Fast-CDR: not installed
```

## Commands verified

```bash
./scripts/build.sh
./scripts/run_all_vm.sh
cd examples
make bin/23_fastdds_packet_codec bin/24_camera_fastdds_packet bin/25_dds_shm_frame_meta bin/26_gstreamer_pipeline_config
make run EXAMPLE=23_fastdds_packet_codec
make run EXAMPLE=24_camera_fastdds_packet
make run EXAMPLE=25_dds_shm_frame_meta
make run EXAMPLE=26_gstreamer_pipeline_config
./scripts/benchmark_gstreamer_camera.sh /dev/video0 5 640 480 30 yuyv
./scripts/run_gstreamer_encode_stream.sh synthetic /dev/video0 5 640 480 30 yuyv h264 logs/gstreamer_output.h264 logs/benchmark_v2_6_gstreamer_camera/gstreamer_encode_stream.csv
```

## Results

```text
build: PASS
main pipeline: status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
example 23: PASS
example 24: PASS
example 25: PASS
example 26: PASS
GStreamer capture node: NOT_COMPILED fallback as expected
GStreamer synthetic fallback: PASS, captured=5, size_errors=0, payload_errors=0
GStreamer encode node: NOT_COMPILED fallback as expected
```

## GStreamer ON attempt

```bash
cmake -S . -B build_gst_on -DAVM_ENABLE_GSTREAMER=ON -DCMAKE_BUILD_TYPE=Release
```

Result:

```text
Package 'gstreamer-1.0', required by 'virtual:world', not found
Package 'gstreamer-app-1.0', required by 'virtual:world', not found
```

Interpretation: source code has dependency-free compilation and fallback validation in this container; real GStreamer appsink/appsrc build and USB camera capture are waiting for user VMware/real Linux environment with GStreamer development packages installed.
