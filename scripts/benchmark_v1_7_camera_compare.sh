#!/usr/bin/env bash
set -euo pipefail

DEV="${1:-/dev/video0}"
FRAMES="${2:-120}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
FPS="${5:-30}"
BACKEND="${6:-dummy}"

mkdir -p logs/benchmark

if [[ ! -e "$DEV" ]]; then
  echo "[benchmark_v1_7_camera_compare] device not found: $DEV" >&2
  echo "[benchmark_v1_7_camera_compare] Run this on VMware Ubuntu or Orange Pi 5 Plus with USB camera attached." >&2
  exit 2
fi

./scripts/run_camera_pipeline.sh "$DEV" "$FRAMES" "$WIDTH" "$HEIGHT" "$FPS" "$BACKEND" rgb logs/benchmark/camera_rgb_perf.csv
cp logs/final_status_camera.txt logs/benchmark/final_status_camera_rgb.txt || true
cp logs/media_camera.log logs/benchmark/media_camera_rgb.log || true
cp logs/preprocess_camera.log logs/benchmark/preprocess_camera_rgb.log || true
cp logs/npu_camera.log logs/benchmark/npu_camera_rgb.log || true
cp logs/safety_camera.log logs/benchmark/safety_camera_rgb.log || true

./scripts/run_camera_pipeline.sh "$DEV" "$FRAMES" "$WIDTH" "$HEIGHT" "$FPS" "$BACKEND" yuyv logs/benchmark/camera_yuyv_fused_perf.csv
cp logs/final_status_camera.txt logs/benchmark/final_status_camera_yuyv.txt || true
cp logs/media_camera.log logs/benchmark/media_camera_yuyv.log || true
cp logs/preprocess_camera.log logs/benchmark/preprocess_camera_yuyv.log || true
cp logs/npu_camera.log logs/benchmark/npu_camera_yuyv.log || true
cp logs/safety_camera.log logs/benchmark/safety_camera_yuyv.log || true

python3 scripts/visualize_perf_compare.py \
  --input logs/benchmark/camera_rgb_perf.csv --case camera_rgb \
  --input logs/benchmark/camera_yuyv_fused_perf.csv --case camera_yuyv_fused \
  --out-csv logs/benchmark/camera_perf_compare_summary.csv \
  --out-html logs/benchmark/camera_perf_compare.html

echo "[benchmark_v1_7_camera_compare] html=logs/benchmark/camera_perf_compare.html"