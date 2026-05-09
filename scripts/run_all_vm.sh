#!/usr/bin/env bash
set -euo pipefail

mkdir -p logs logs/preprocess logs/frames

./scripts/clean_ipc.sh || true
./scripts/prepare_input.sh

./build/control_service > logs/control_service.log 2>&1 &
PID_CTRL=$!

# 先启动消费者。preprocess_node 会等待 media_node 创建 FramePool；
# npu_stub_node 会等待 preprocess_node 创建 TensorMeta Ring。
./build/preprocess_node --frames 120 --save-every 60 \
    > logs/preprocess_node.log 2>&1 &
PID_PRE=$!

./build/npu_stub_node --frames 120 --fake-latency 8 \
    > logs/npu_stub_node.log 2>&1 &
PID_NPU=$!

# 给消费者一个很短的启动时间，但不能让生产者先跑太久。
sleep 0.2

# 再启动生产者，避免 FramePool 槽位在消费者未启动前被覆盖。
./build/media_node --source file --input assets/input_640x480_rgb.raw --frames 120 \
    > logs/media_node.log 2>&1 &
PID_MEDIA=$!

# 最后启动 safety_monitor，避免因节点尚未 heartbeat 而误报 timeout。
sleep 1
./build/safety_monitor --duration 15 \
    > logs/safety_monitor.log 2>&1 &
PID_SAFE=$!

wait "$PID_MEDIA" || true
wait "$PID_PRE" || true
wait "$PID_NPU" || true

sleep 1

echo "=== Final Status ==="
./build/control_client QUERY_STATUS | tee logs/final_status.txt || true

kill "$PID_SAFE" "$PID_CTRL" 2>/dev/null || true

echo "[run_all_vm] finished. Check logs/*.log and logs/final_status.txt"