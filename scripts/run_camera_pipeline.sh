#!/usr/bin/env bash
set -euo pipefail

DEV="${1:-/dev/video0}"
FRAMES="${2:-120}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
FPS="${5:-30}"
BACKEND="${6:-dummy}"
CAMERA_OUTPUT="${7:-rgb}"   # rgb | yuyv

mkdir -p logs logs/preprocess logs/frames

./scripts/clean_ipc.sh || true

./build/control_service > logs/control_service_camera.log 2>&1 &
PID_CTRL=$!

./build/preprocess_node --frames "$FRAMES" --save-every 60 > logs/preprocess_camera.log 2>&1 &
PID_PRE=$!

./build/npu_stub_node --frames "$FRAMES" --fake-latency 8 --backend "$BACKEND" > logs/npu_camera.log 2>&1 &
PID_NPU=$!

sleep 0.5

./build/media_node --source camera --device "$DEV" --width "$WIDTH" --height "$HEIGHT" --fps "$FPS" --frames "$FRAMES" --camera-output "$CAMERA_OUTPUT" \
  > logs/media_camera.log 2>&1 &
PID_MEDIA=$!

sleep 1

./build/safety_monitor --duration 20 > logs/safety_camera.log 2>&1 &
PID_SAFE=$!

wait "$PID_MEDIA" || true
wait "$PID_PRE" || true
wait "$PID_NPU" || true

sleep 1

./build/control_client QUERY_STATUS | tee logs/final_status_camera.txt || true

kill "$PID_SAFE" "$PID_CTRL" 2>/dev/null || true

echo "[run_camera_pipeline] done. camera_output=$CAMERA_OUTPUT. Check logs/final_status_camera.txt and logs/*_camera.log"
