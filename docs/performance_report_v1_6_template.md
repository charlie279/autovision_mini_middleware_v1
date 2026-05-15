# AutoVision Mini Middleware V1.6 Performance Report Template

## 1. Test Environment

| Item | Value |
|---|---|
| Host / Board | TODO |
| OS | TODO |
| Camera device | /dev/video0 |
| Camera format | YUYV 640x480@30fps |
| Runtime backend | dummy / rknn_stub |
| Test frames | 900 |

## 2. RGB vs YUYV Fused Path

| Metric | V1.5-compatible RGB path | V1.6 YUYV fused path | Delta |
|---|---:|---:|---:|
| input payload/frame | 921600 B | 614400 B | -33.3% |
| input payload bandwidth @30fps | 27.65 MB/s | 18.43 MB/s | -9.22 MB/s |
| preprocess mean | TODO | TODO | TODO |
| preprocess p95 | TODO | TODO | TODO |
| total mean | TODO | TODO | TODO |
| total p95 | TODO | TODO | TODO |
| media_frames | TODO | TODO | should match |
| preprocess_frames | TODO | TODO | should match |
| npu_frames | TODO | TODO | should match |
| crc_errors | TODO | TODO | expected 0 |
| frame_jumps | TODO | TODO | expected 0 |
| unsupported_format | TODO | TODO | expected 0 |

## 3. Commands

```bash
./scripts/benchmark_v1_6_camera_compare.sh /dev/video0 900 640 480 30 dummy
```

## 4. Log Extraction

```bash
grep -E "Preprocess|finished" logs/benchmark_v1_6/rgb/preprocess_camera.log
grep -E "Preprocess|finished" logs/benchmark_v1_6/yuyv/preprocess_camera.log
cat logs/benchmark_v1_6/rgb/final_status_camera.txt
cat logs/benchmark_v1_6/yuyv/final_status_camera.txt
```

## 5. Notes

- V1.6 is still POSIX SHM + memcpy, not DMA-BUF zero-copy.
- The confirmed optimization is payload size reduction and fused CPU preprocessing boundary cleanup.
- Real CPU and latency gains must be reported from VMware / Orange Pi 5 Plus measurements.
