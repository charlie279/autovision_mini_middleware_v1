#!/usr/bin/env bash
set -euo pipefail

SOURCE="${1:-synthetic}"
DEVICE="${2:-/dev/video0}"
FRAMES="${3:-120}"
WIDTH="${4:-640}"
HEIGHT="${5:-480}"
FPS="${6:-30}"
FORMAT="${7:-yuyv}"
CODEC="${8:-h264}"
ENCODED_OUTPUT="${9:-logs/gstreamer_output.h264}"
CSV_OUTPUT="${10:-logs/benchmark_v2_6_gstreamer_camera/gstreamer_encode_stream.csv}"

mkdir -p "$(dirname "$ENCODED_OUTPUT")" "$(dirname "$CSV_OUTPUT")"
./build/gstreamer_encode_stream_node \
  --source "$SOURCE" \
  --device "$DEVICE" \
  --frames "$FRAMES" \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --fps "$FPS" \
  --format "$FORMAT" \
  --codec "$CODEC" \
  --encoded-output "$ENCODED_OUTPUT" \
  --output "$CSV_OUTPUT" \
  --allow-unavailable

echo "[run_gstreamer_encode_stream] csv=$CSV_OUTPUT output=$ENCODED_OUTPUT"
