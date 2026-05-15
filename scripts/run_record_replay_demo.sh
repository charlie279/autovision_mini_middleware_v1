#!/usr/bin/env bash
set -euo pipefail
FRAMES=${1:-30}
REC=${2:-logs/record_replay_demo.avmr}
./scripts/clean_ipc.sh
./scripts/prepare_input.sh
./build/media_node --source file --frames "$FRAMES" --record "$REC" > logs/record_media.log 2>&1
./scripts/clean_ipc.sh
./build/media_node --source replay --frames "$FRAMES" --replay "$REC" > logs/replay_media.log 2>&1 &
MEDIA_PID=$!
./build/preprocess_node --frames "$FRAMES" --save-every 0 --perf-log logs/replay_preprocess.csv > logs/replay_preprocess.log 2>&1 &
PRE_PID=$!
./build/npu_stub_node --frames "$FRAMES" --backend dummy > logs/replay_npu.log 2>&1 &
NPU_PID=$!
wait "$MEDIA_PID"
wait "$PRE_PID"
wait "$NPU_PID"
./build/algorithm_stub_node --input logs/npu_results.jsonl --output logs/detection_results.jsonl > logs/algorithm_stub.log 2>&1
cat logs/replay_media.log | tail -n 2
cat logs/replay_preprocess.log | tail -n 2
cat logs/replay_npu.log | tail -n 2
cat logs/algorithm_stub.log | tail -n 2
