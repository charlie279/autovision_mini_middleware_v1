#!/usr/bin/env bash
set -euo pipefail
mkdir -p logs
case "${1:-}" in
  bad_crc)
    ./scripts/clean_ipc.sh || true
    ./scripts/prepare_input.sh
    ./build/safety_monitor --duration 20 > logs/safety_bad_crc.log 2>&1 &
    PID_SAFE=$!
    ./build/preprocess_node --frames 120 > logs/preprocess_bad_crc.log 2>&1 &
    PID_PRE=$!
    ./build/npu_stub_node --frames 120 > logs/npu_bad_crc.log 2>&1 &
    PID_NPU=$!
    sleep 1
    ./build/media_node --source file --input assets/input_640x480_rgb.raw --frames 120 --inject-bad-crc \
      > logs/media_bad_crc.log 2>&1
    wait "$PID_PRE" || true
    wait "$PID_NPU" || true
    sleep 2
    kill "$PID_SAFE" 2>/dev/null || true
    echo "[inject_fault] bad_crc done. Check logs/safety_bad_crc.log and logs/preprocess_bad_crc.log"
    ;;
  frame_jump)
    ./scripts/clean_ipc.sh || true
    ./scripts/prepare_input.sh
    ./build/safety_monitor --duration 20 > logs/safety_frame_jump.log 2>&1 &
    PID_SAFE=$!
    ./build/preprocess_node --frames 120 > logs/preprocess_frame_jump.log 2>&1 &
    PID_PRE=$!
    ./build/npu_stub_node --frames 120 > logs/npu_frame_jump.log 2>&1 &
    PID_NPU=$!
    sleep 1
    ./build/media_node --source file --input assets/input_640x480_rgb.raw --frames 120 --inject-frame-jump \
      > logs/media_frame_jump.log 2>&1
    wait "$PID_PRE" || true
    wait "$PID_NPU" || true
    sleep 2
    kill "$PID_SAFE" 2>/dev/null || true
    echo "[inject_fault] frame_jump done. Check logs/safety_frame_jump.log and logs/preprocess_frame_jump.log"
    ;;
  *)
    echo "usage: $0 {bad_crc|frame_jump}"
    exit 1
    ;;
esac