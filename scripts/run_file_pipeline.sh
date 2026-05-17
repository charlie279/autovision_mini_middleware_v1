#!/usr/bin/env bash
set -euo pipefail

FRAMES="${1:-120}"
BACKEND="${2:-dummy}"
PERF_LOG="${3:-logs/benchmark/file_rgb_perf.csv}"

mkdir -p logs logs/preprocess logs/frames logs/benchmark

./scripts/clean_ipc.sh || true
./scripts/prepare_input.sh

./build/control_service > logs/control_service_file.log 2>&1 &
PID_CTRL=$!

./build/preprocess_node --frames "$FRAMES" --save-every 60 --perf-log "$PERF_LOG" > logs/preprocess_file.log 2>&1 &
PID_PRE=$!

./build/npu_stub_node --frames "$FRAMES" --fake-latency 8 --backend "$BACKEND" > logs/npu_file.log 2>&1 &
PID_NPU=$!

sleep 0.2

./build/media_node --source file --input assets/input_640x480_rgb.raw --frames "$FRAMES" > logs/media_file.log 2>&1 &
PID_MEDIA=$!

sleep 1
./build/safety_monitor --duration 15 > logs/safety_file.log 2>&1 &
PID_SAFE=$!

wait "$PID_MEDIA" || true
wait "$PID_PRE" || true
wait "$PID_NPU" || true

sleep 1
./build/control_client QUERY_STATUS | tee logs/final_status_file.txt || true

kill "$PID_SAFE" "$PID_CTRL" 2>/dev/null || true

echo "[run_file_pipeline] done. Check logs/final_status_file.txt and $PERF_LOG"
