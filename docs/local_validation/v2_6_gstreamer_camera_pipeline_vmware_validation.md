# V2.6 GStreamer Camera Pipeline VMware Validation

## 1. Validation Environment

- Host environment: VMware Ubuntu
- Project branch: `v2.6-gstreamer-camera-pipeline`
- Compiler: GNU 11.4.0
- Camera device: `/dev/video0`
- Camera driver: UVC / V4L2
- Camera format used in validation:
  - Pixel format: YUYV / YUY2
  - Resolution: 640 × 480
  - FPS: 30

## 2. GStreamer Runtime and Plugin Check

The following GStreamer elements were verified with `gst-inspect-1.0`.

| Element | Plugin / Function | Result |
|---|---|---|
| `v4l2src` | V4L2 camera source | PASS |
| `appsink` | Application-side raw buffer sink | PASS |
| `appsrc` | Application-side raw buffer source | PASS |
| `videoconvert` | Video colorspace converter | PASS |
| `x264enc` | H.264 encoder | PASS |
| `h264parse` | H.264 parser | PASS |
| `x265enc` | H.265 encoder | PASS |

Detected GStreamer versions:

```text
gstreamer-1.0: 1.20.3
gstreamer-app-1.0: 1.20.1
````

The key plugins were found in the local system:

```text
v4l2src:    /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstvideo4linux2.so
appsink:    /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstapp.so
appsrc:     /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstapp.so
x264enc:    /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstx264.so
h264parse:  /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstvideoparsersbad.so
x265enc:    /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstx265.so
```

## 3. Camera Device Check

Command:

```bash
ls -l /dev/video*
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 --list-formats-ext
```

Detected device:

```text
Web Camera: Web Camera (usb-0000:03:00.0-2):
        /dev/video0
        /dev/video1
        /dev/media0
```

Supported formats included:

```text
[0]: 'MJPG' (Motion-JPEG, compressed)
[1]: 'YUYV' (YUYV 4:2:2)
```

The validation path used:

```text
YUYV 640x480 @ 30 fps
```

Relevant capability:

```text
[1]: 'YUYV' (YUYV 4:2:2)
        Size: Discrete 640x480
                Interval: Discrete 0.033s (30.000 fps)
```

## 4. Default Dependency-Free Validation

Before enabling GStreamer, the default build was verified to ensure that V2.6 does not introduce a hard dependency on GStreamer.

### 4.1 Default Build

Command:

```bash
rm -rf build
./scripts/build.sh
```

Result:

```text
Built target avm_gstreamer
Built target gstreamer_camera_capture_node
Built target gstreamer_encode_stream_node
[build] done
```

Status: PASS

### 4.2 Main Pipeline Regression

Command:

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

Result:

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

Status: PASS

### 4.3 Examples 23 / 24 / 25 / 26

Commands:

```bash
cd examples

make clean || true

make bin/23_fastdds_packet_codec \
     bin/24_camera_fastdds_packet \
     bin/25_dds_shm_frame_meta \
     bin/26_gstreamer_pipeline_config

make run EXAMPLE=23_fastdds_packet_codec
make run EXAMPLE=24_camera_fastdds_packet
make run EXAMPLE=25_dds_shm_frame_meta
make run EXAMPLE=26_gstreamer_pipeline_config
```

Results:

```text
[23_fastdds_packet_codec] encoded_bytes=160096 kind=lidar seq=7 payload=160000 decode=OK validate=OK result=PASS
[24_camera_fastdds_packet] yuyv_payload=614400 yuyv_packet=614496 rgb_payload=921600 rgb_packet=921696 v24_qos_yuyv=OK v24_qos_rgb=OK old_512k_qos=EXPECTED_FAIL result=PASS
[25_dds_shm_frame_meta] metadata_bytes=208 raw_payload_bytes=614400 size_errors=0 payload_errors=0 result=PASS
[26_gstreamer_pipeline_config] raw_payload_bytes=614400 capture_ok=1 encode_ok=1 size_ok=1 result=PASS
```

Status: PASS

### 4.4 GStreamer Disabled Fallback

Command:

```bash
./scripts/benchmark_gstreamer_camera.sh /dev/video0 5 640 480 30 yuyv
```

Real GStreamer capture path result when GStreamer was not enabled:

```text
role,compiled,device,format,width,height,fps,frames_requested,captured,size_errors,payload_errors,mean_interval_ms,p95_interval_ms,last_crc32,status
capture,false,/dev/video0,yuyv,640,480,30,5,0,0,0,0,0,0,NOT_COMPILED
```

Synthetic fallback result:

```text
role,compiled,device,format,width,height,fps,frames_requested,captured,size_errors,payload_errors,mean_interval_ms,p95_interval_ms,last_crc32,status
capture,0,/dev/video0,yuyv,640,480,30,5,5,0,0,37.219,37.960,1684164400,PASS
```

Status: PASS

### 4.5 GStreamer Encode Disabled Fallback

Command:

```bash
./scripts/run_gstreamer_encode_stream.sh synthetic /dev/video0 10 640 480 30 yuyv h264 logs/gstreamer_output.h264
```

Result:

```text
role,compiled,source,device,format,width,height,fps,frames_requested,pushed,output,output_bytes,status
encode,false,synthetic,/dev/video0,yuyv,640,480,30,10,0,logs/gstreamer_output.h264,0,NOT_COMPILED
```

Status: PASS

Conclusion: when GStreamer is not enabled, the new V2.6 binaries correctly report `NOT_COMPILED` instead of producing a false success result.

## 5. GStreamer ON Build Validation

Command:

```bash
rm -rf build
./scripts/build.sh -DAVM_ENABLE_GSTREAMER=ON
```

CMake detected GStreamer correctly:

```text
-- Checking for modules 'gstreamer-1.0;gstreamer-app-1.0'
--   Found gstreamer-1.0, version 1.20.3
--   Found gstreamer-app-1.0, version 1.20.1
```

Build result:

```text
Built target avm_gstreamer
Built target gstreamer_camera_capture_node
Built target gstreamer_encode_stream_node
[build] done
```

Status: PASS

Warnings observed during build:

```text
warning: ignoring return value of write(...)
warning: unused variable 'last_alive_counter'
```

These warnings do not block the V2.6 validation and do not affect the GStreamer Camera Pipeline result.

## 6. Real USB Camera GStreamer Capture Validation

### 6.1 Command

```bash
./scripts/run_gstreamer_camera_capture.sh /dev/video0 300 640 480 30 yuyv
```

### 6.2 Pipeline

```text
v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 ! queue max-size-buffers=4 leaky=downstream ! appsink name=appsink sync=false drop=true max-buffers=4
```

### 6.3 Result

```text
[gstreamer_camera_capture_node] captured=300 size_errors=0 payload_errors=0 mean_interval_ms=33.919 p95_interval_ms=40.060 result=PASS csv=logs/benchmark_v2_6_gstreamer_camera/gstreamer_camera_capture.csv
```

CSV result:

```text
role,compiled,device,format,width,height,fps,frames_requested,captured,size_errors,payload_errors,mean_interval_ms,p95_interval_ms,last_crc32,status
capture,1,/dev/video0,yuyv,640,480,30,300,300,0,0,33.919,40.060,1797370295,PASS
```

### 6.4 Acceptance Check

| Metric           |     Expected | Actual | Result |
| ---------------- | -----------: | -----: | ------ |
| `compiled`       | `true` / `1` |    `1` | PASS   |
| `captured`       |          300 |    300 | PASS   |
| `size_errors`    |            0 |      0 | PASS   |
| `payload_errors` |            0 |      0 | PASS   |
| `status`         |         PASS |   PASS | PASS   |

Conclusion: real USB Camera capture through `v4l2src -> caps -> queue -> appsink -> GStreamerCameraAdapter -> SensorFrame` is validated.

## 7. H.264 Appsrc Encode Validation

### 7.1 Command

```bash
./scripts/run_gstreamer_encode_stream.sh synthetic /dev/video0 120 640 480 30 yuyv h264 logs/gstreamer_output.h264
```

### 7.2 Pipeline

```text
appsrc name=appsrc is-live=true format=time do-timestamp=true block=true caps=video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 ! videoconvert ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=4000 key-int-max=30 ! h264parse ! filesink location=logs/gstreamer_output.h264 sync=false
```

### 7.3 Result

```text
[gstreamer_encode_stream_node] pushed=120 output=logs/gstreamer_output.h264 output_bytes=2062186 result=PASS csv=logs/benchmark_v2_6_gstreamer_camera/gstreamer_encode_stream.csv
```

CSV result:

```text
role,compiled,source,device,format,width,height,fps,frames_requested,pushed,output,output_bytes,status
encode,true,synthetic,/dev/video0,yuyv,640,480,30,120,120,logs/gstreamer_output.h264,2062186,PASS
```

Output file:

```text
logs/gstreamer_output.h264
size: 2.0M
```

### 7.4 Acceptance Check

| Metric         | Expected |  Actual | Result |
| -------------- | -------: | ------: | ------ |
| `compiled`     |     true |    true | PASS   |
| `pushed`       |      120 |     120 | PASS   |
| `output_bytes` |      > 0 | 2062186 | PASS   |
| `status`       |     PASS |    PASS | PASS   |

Conclusion: synthetic `SensorFrame -> appsrc -> videoconvert -> x264enc -> h264parse -> filesink` path is validated.

## 8. H.265 Appsrc Encode Validation

### 8.1 Plugin Check

Command:

```bash
gst-inspect-1.0 x265enc
```

Result:

```text
Factory Details:
  Long-name                x265enc
  Klass                    Codec/Encoder/Video
  Description              H265 Encoder

Plugin Details:
  Name                     x265
  Version                  1.20.3
  Source module            gst-plugins-bad
```

Status: PASS

### 8.2 Command

```bash
./scripts/run_gstreamer_encode_stream.sh synthetic /dev/video0 120 640 480 30 yuyv h265 logs/gstreamer_output.h265
```

### 8.3 Pipeline

```text
appsrc name=appsrc is-live=true format=time do-timestamp=true block=true caps=video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 ! videoconvert ! x265enc tune=zerolatency bitrate=4000 key-int-max=30 ! h265parse ! filesink location=logs/gstreamer_output.h265 sync=false
```

### 8.4 Result

```text
[gstreamer_encode_stream_node] pushed=120 output=logs/gstreamer_output.h265 output_bytes=268912 result=PASS csv=logs/benchmark_v2_6_gstreamer_camera/gstreamer_encode_stream.csv
```

CSV result:

```text
role,compiled,source,device,format,width,height,fps,frames_requested,pushed,output,output_bytes,status
encode,true,synthetic,/dev/video0,yuyv,640,480,30,120,120,logs/gstreamer_output.h265,268912,PASS
```

Output file:

```text
logs/gstreamer_output.h265
size: 263K
```

### 8.5 Acceptance Check

| Metric         | Expected | Actual | Result |
| -------------- | -------: | -----: | ------ |
| `compiled`     |     true |   true | PASS   |
| `pushed`       |      120 |    120 | PASS   |
| `output_bytes` |      > 0 | 268912 | PASS   |
| `status`       |     PASS |   PASS | PASS   |

Conclusion: synthetic `SensorFrame -> appsrc -> videoconvert -> x265enc -> h265parse -> filesink` path is validated.

## 9. Overall V2.6 Validation Summary

| Validation Item                             | Result |
| ------------------------------------------- | ------ |
| Default dependency-free build               | PASS   |
| Main pipeline regression `run_all_vm.sh`    | PASS   |
| Examples 23 / 24 / 25 / 26                  | PASS   |
| GStreamer disabled fallback                 | PASS   |
| Synthetic fallback capture                  | PASS   |
| GStreamer ON build                          | PASS   |
| Real USB Camera appsink capture, 300 frames | PASS   |
| H.264 appsrc encode, 120 frames             | PASS   |
| H.265 appsrc encode, 120 frames             | PASS   |

## 10. Engineering Conclusion

V2.6 successfully extends the V2.5 DDS + SHM mixed-route baseline with an optional GStreamer Camera Pipeline.

The following paths were validated on VMware Ubuntu:

```text
USB Camera /dev/video0
    -> GStreamer v4l2src
    -> video/x-raw capsfilter
    -> queue
    -> appsink name=appsink
    -> GStreamerCameraAdapter
    -> AutoVision SensorFrame
```

```text
Synthetic SensorFrame
    -> appsrc name=appsrc
    -> videoconvert
    -> x264enc / x265enc
    -> h264parse / h265parse
    -> filesink
```

The V2.6 implementation keeps GStreamer optional. When GStreamer is not enabled, the binaries explicitly report `NOT_COMPILED`; when GStreamer is enabled, both real camera capture and appsrc encode paths pass validation.

## 11. Scope Boundary

The following items are not claimed as completed in V2.6:

1. True DMA-BUF zero-copy is not completed.
2. The current `--use-dmabuf` path only requests `v4l2src io-mode=dmabuf`.
3. The appsink side still maps/copies the frame payload into `SensorFrame.data`.
4. FastDDS typed IDL `DataWriter/DataReader` is not implemented in this version.
5. Cross-host SHM is not supported.
6. Real AXera / RKNN / board-side SDK integration is reserved for later versions.

## 12. Recommended Next Step

The recommended next version is V2.7:

```text
camera_dds_shm_pub --source gstreamer
```

Target:

```text
GStreamerCameraAdapter
    -> SensorFrame
    -> POSIX SHM raw payload
    -> SharedFrameDescriptor metadata
    -> FastDDS / RTPS metadata topic
    -> camera_dds_shm_sub
```

This will connect the V2.6 GStreamer camera source directly into the V2.5 DDS + SHM mixed-route path.

```
```
