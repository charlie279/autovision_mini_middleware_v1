#!/usr/bin/env bash
set -euo pipefail

DEVICE="${1:-/dev/video0}"
FRAMES="${2:-300}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
FPS="${5:-30}"
FORMAT="${6:-yuyv}"
OUTPUT="${7:-logs/benchmark_v2_6_gstreamer_camera/gstreamer_camera_capture.csv}"

mkdir -p "$(dirname "$OUTPUT")"
./build/gstreamer_camera_capture_node \
  --source gstreamer \
  --device "$DEVICE" \
  --frames "$FRAMES" \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --fps "$FPS" \
  --format "$FORMAT" \
  --output "$OUTPUT" \
  --allow-unavailable

echo "[run_gstreamer_camera_capture] csv=$OUTPUT"
