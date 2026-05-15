#!/usr/bin/env bash
set -euo pipefail

mkdir -p logs logs/preprocess logs/frames logs/benchmark

./scripts/clean_ipc.sh || true
./scripts/prepare_input.sh

./build/control_service > logs/control_service.log 2>&1 &
PID_CTRL=$!

# 先启动消费者节点，让它们等待生产者创建共享内存对象。
# 这可以避免 media_node 先跑太久导致 FramePool 的 8 个槽位被覆盖。
./build/preprocess_node --frames 120 --save-every 60 --perf-log logs/benchmark/file_rgb_perf.csv > logs/preprocess_node.log 2>&1 &
PID_PRE=$!

./build/npu_stub_node --frames 120 --fake-latency 8 > logs/npu_stub_node.log 2>&1 &
PID_NPU=$!

sleep 0.2

./build/media_node --source file --input assets/input_640x480_rgb.raw --frames 120 \
    > logs/media_node.log 2>&1 &
PID_MEDIA=$!

# Safety Monitor 最后启动，避免在节点尚未 heartbeat 时误报 timeout。
sleep 1
./build/safety_monitor --duration 15 > logs/safety_monitor.log 2>&1 &
PID_SAFE=$!

wait "$PID_MEDIA" || true
wait "$PID_PRE" || true
wait "$PID_NPU" || true

sleep 1

echo "=== Final Status ==="
./build/control_client QUERY_STATUS | tee logs/final_status.txt || true

kill "$PID_SAFE" "$PID_CTRL" 2>/dev/null || true

echo "[run_all_vm] finished. Check logs/*.log and logs/final_status.txt"