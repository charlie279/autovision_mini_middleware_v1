#!/usr/bin/env bash
set -euo pipefail

DEV="${1:-/dev/video0}"
OUTDIR="logs/camera_check"
mkdir -p "$OUTDIR"

pkill -9 ffplay 2>/dev/null || true
pkill -9 cheese 2>/dev/null || true
pkill -9 guvcview 2>/dev/null || true
sudo fuser -k "$DEV" 2>/dev/null || true
sleep 1

{
  echo "=== date ==="
  date
  echo
  echo "=== groups ==="
  groups
  echo
  echo "=== lsusb ==="
  lsusb
  echo
  echo "=== lsusb -t ==="
  lsusb -t
  echo
  echo "=== video nodes ==="
  ls -l /dev/video*
  echo
  echo "=== devices ==="
  v4l2-ctl --list-devices
  echo
  echo "=== all ==="
  v4l2-ctl -d "$DEV" --all
  echo
  echo "=== formats ==="
  v4l2-ctl -d "$DEV" --list-formats-ext
} | tee "$OUTDIR/device_capability.log"

echo "[TEST] YUYV 640x480@30"
v4l2-ctl -d "$DEV" --set-fmt-video=width=640,height=480,pixelformat=YUYV --set-parm=30
v4l2-ctl -d "$DEV" --get-fmt-video --get-parm | tee "$OUTDIR/yuyv_640_get_fmt.log"

rm -f /tmp/usb_cam_yuyv_640x480_60f.yuyv
timeout 10s v4l2-ctl -d "$DEV" \
  --stream-mmap=4 \
  --stream-count=60 \
  --stream-poll \
  --stream-to=/tmp/usb_cam_yuyv_640x480_60f.yuyv \
  --verbose 2>&1 | tee "$OUTDIR/yuyv_640x480_60f.log" || true

if [ -f /tmp/usb_cam_yuyv_640x480_60f.yuyv ]; then
  YUYV_SIZE=$(stat -c "%s" /tmp/usb_cam_yuyv_640x480_60f.yuyv)
else
  YUYV_SIZE=0
fi

echo "[TEST] MJPG 640x480@30"
v4l2-ctl -d "$DEV" --set-fmt-video=width=640,height=480,pixelformat=MJPG --set-parm=30 || true
v4l2-ctl -d "$DEV" --get-fmt-video --get-parm | tee "$OUTDIR/mjpg_640_get_fmt.log" || true

rm -f /tmp/usb_cam_mjpg_640x480_120f.bin
timeout 15s v4l2-ctl -d "$DEV" \
  --stream-mmap=4 \
  --stream-skip=5 \
  --stream-count=120 \
  --stream-poll \
  --stream-to=/tmp/usb_cam_mjpg_640x480_120f.bin \
  --verbose 2>&1 | tee "$OUTDIR/mjpg_640x480_120f.log" || true

if [ -f /tmp/usb_cam_mjpg_640x480_120f.bin ]; then
  MJPG_SIZE=$(stat -c "%s" /tmp/usb_cam_mjpg_640x480_120f.bin)
else
  MJPG_SIZE=0
fi

sudo dmesg -T | tail -n 150 | grep -iE "uvc|video|usb|error|timeout|reset|bandwidth|urb" \
  | tee "$OUTDIR/dmesg_after_tests.log" || true

{
  echo "# Camera V1.5 Check Summary"
  echo
  echo "- device: $DEV"
  echo "- YUYV expected size for 60 frames: 36864000 bytes"
  echo "- YUYV actual size: $YUYV_SIZE"
  echo "- MJPG actual size: $MJPG_SIZE"
  echo
  if [ "$YUYV_SIZE" = "36864000" ]; then
    echo "Conclusion: YUYV 640x480@30fps mmap capture passed. Use YUYV as V1.5 main path."
  else
    echo "Conclusion: YUYV size mismatch. Check logs before coding CameraAdapter."
  fi
} | tee "$OUTDIR/summary.md"