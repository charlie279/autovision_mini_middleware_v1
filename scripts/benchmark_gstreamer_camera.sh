#!/usr/bin/env bash
set -euo pipefail

DEVICE="${1:-/dev/video0}"
FRAMES="${2:-300}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
FPS="${5:-30}"
FORMAT="${6:-yuyv}"
OUT_DIR="logs/benchmark_v2_6_gstreamer_camera"
mkdir -p "$OUT_DIR"

./build/gstreamer_camera_capture_node \
  --source gstreamer \
  --device "$DEVICE" \
  --frames "$FRAMES" \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --fps "$FPS" \
  --format "$FORMAT" \
  --output "$OUT_DIR/gstreamer_camera_capture.csv" \
  --allow-unavailable

# Dependency-free sanity path, useful in containers without GStreamer dev/runtime or a real USB camera.
./build/gstreamer_camera_capture_node \
  --source synthetic \
  --frames 5 \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --fps "$FPS" \
  --format "$FORMAT" \
  --output "$OUT_DIR/gstreamer_camera_synthetic_fallback.csv"

echo "[benchmark_gstreamer_camera] logs=$OUT_DIR"
