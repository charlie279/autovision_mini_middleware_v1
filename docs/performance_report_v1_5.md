# AutoVision Mini Middleware V1.5 Performance Report

## 1. Environment

| Item | Value |
|---|---|
| Host | Windows + VMware |
| Guest OS | Ubuntu |
| VMware USB compatibility | USB 3.1 |
| Camera device | /dev/video0 |
| Driver | uvcvideo |
| Main format | YUYV 640x480 |
| Runtime backend | dummy |

## 2. Camera Validation

| Item | Expected | Actual | Result |
|---|---:|---:|---|
| YUYV 60-frame file size | 36,864,000 bytes | TODO | TODO |
| YUYV ffplay raw playback | normal | TODO | TODO |
| Realtime ffplay YUYV | normal | TODO | TODO |
| dmesg URB timeout/reset | none | TODO | TODO |

## 3. Example 11

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh
cd examples
make run EXAMPLE=11_camera_adapter_v4l2
```

Expected:

```text
frame_id=1 sensor_type=0 width=640 height=480 data_size=921600
saved examples/logs/camera_first_frame.ppm
```

## 4. Camera Pipeline

```bash
./scripts/run_camera_pipeline.sh /dev/video0 120 640 480 30 dummy
cat logs/final_status_camera.txt
```

Expected:

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## 5. Runtime Results

Check:

```bash
tail -n 5 logs/npu_results.jsonl
```

Expected JSON fields:

```text
frame_id
object_count
confidence
npu_latency_ms
backend
```

## 6. Known Limitations

```text
1. V1.5 still uses POSIX SHM + memcpy, not DMA-BUF zero-copy.
2. CameraAdapter currently outputs RGB888 to preserve existing preprocess_node path.
3. RKNN backend is stub-only and does not link real RKNN SDK.
4. MJPEG fallback is documented but not implemented in this patch package.
```