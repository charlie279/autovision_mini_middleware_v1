#!/usr/bin/env bash
set -euo pipefail

DEVICE="${1:-/dev/video0}"
FRAMES="${2:-300}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
FPS="${5:-30}"
mkdir -p logs/benchmark

if ! command -v gst-launch-1.0 >/dev/null 2>&1; then
  echo "[benchmark_gstreamer_camera] gst-launch-1.0 not found. Install: sudo apt install -y gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good"
  exit 0
fi

if [[ ! -e "$DEVICE" ]]; then
  echo "[benchmark_gstreamer_camera] device not found: $DEVICE"
  exit 0
fi

gst-launch-1.0 -v v4l2src device="$DEVICE" num-buffers="$FRAMES" \
  ! "video/x-raw,format=YUY2,width=${WIDTH},height=${HEIGHT},framerate=${FPS}/1" \
  ! queue max-size-buffers=4 leaky=downstream \
  ! fpsdisplaysink video-sink=fakesink sync=false \
  2>&1 | tee logs/benchmark/gstreamer_camera_fps.log

echo "[benchmark_gstreamer_camera] log=logs/benchmark/gstreamer_camera_fps.log"
