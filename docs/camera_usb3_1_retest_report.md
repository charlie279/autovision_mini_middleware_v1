# Camera USB3.1 Retest Report

## 1. Current Result

VMware USB compatibility has been switched to USB 3.1. The external USB UVC camera is visible as a V4L2 capture device and YUYV 640x480 mmap capture has passed functional validation.

## 2. Device Summary

| Item | Value |
|---|---|
| Main video node | /dev/video0 |
| Auxiliary node | /dev/video1 |
| Driver | uvcvideo |
| USB path | xhci_hcd |
| UVC link speed | 480M |

## 3. YUYV Main Path

Observed target configuration:

```text
Pixel Format: YUYV
Resolution: 640x480
FPS: 30
Bytes per Line: 1280
Size Image: 614400
```

60-frame mmap capture size:

```text
640 * 480 * 2 * 60 = 36,864,000 bytes
```

Decision:

```text
Use YUYV mmap as V1.5 main CameraAdapterV4L2 path.
```

## 4. MJPEG Fallback

MJPEG can be captured and kept as a future fallback. It is not part of the first code patch because the current V1.5 path should avoid additional JPEG decode dependency.