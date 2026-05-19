#!/usr/bin/env bash
set -euo pipefail
FRAMES="${1:-60}"
WIDTH="${2:-640}"
HEIGHT="${3:-480}"
FPS="${4:-30}"
FORMAT="${5:-yuyv}"
TOPIC="${6:-avm/camera/dds_shm_synthetic_meta}"
SHM_NAME="${7:-/avm_camera_dds_shm_synthetic_pool}"
DEPTH="${8:-1024}"
REPEAT="${9:-1}"
WARMUP="${10:-10}"
POOL_COUNT="${11:-64}"
BPP=2
if [[ "$FORMAT" == "rgb" || "$FORMAT" == "rgb888" ]]; then BPP=3; fi
BUFFER_SIZE=$((WIDTH * HEIGHT * BPP))
START_SEQ=$((WARMUP + 1))
OUT_DIR="logs/benchmark_v2_5_dds_shm_synthetic"
mkdir -p "$OUT_DIR"
rm -f "/dev/shm/${SHM_NAME#/}"

./build/camera_dds_shm_sub \
  --topic "$TOPIC" \
  --shm-name "$SHM_NAME" \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --format "$FORMAT" \
  --frames "$FRAMES" \
  --start-seq "$START_SEQ" \
  --depth "$DEPTH" \
  --max-payload 8192 \
  --timeout-ms 120000 \
  --reliable \
  --output "$OUT_DIR/camera_dds_shm_sub.csv" \
  --allow-unavailable > "$OUT_DIR/camera_dds_shm_sub.log" 2>&1 &
SUB_PID=$!
sleep 5

PUB_RC=0
./build/camera_dds_shm_pub \
  --source synthetic \
  --topic "$TOPIC" \
  --shm-name "$SHM_NAME" \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --fps "$FPS" \
  --format "$FORMAT" \
  --frames "$FRAMES" \
  --depth "$DEPTH" \
  --max-payload 8192 \
  --pool-count "$POOL_COUNT" \
  --buffer-size "$BUFFER_SIZE" \
  --reliable \
  --startup-ms 5000 \
  --linger-ms 2000 \
  --warmup-frames "$WARMUP" \
  --repeat "$REPEAT" \
  --inter-repeat-us 1000 \
  --output "$OUT_DIR/camera_dds_shm_pub.csv" \
  --allow-unavailable > "$OUT_DIR/camera_dds_shm_pub.log" 2>&1 || PUB_RC=$?
SUB_RC=0
wait "$SUB_PID" || SUB_RC=$?

cat "$OUT_DIR/camera_dds_shm_pub.csv"
cat "$OUT_DIR/camera_dds_shm_sub.csv"
exit $(( PUB_RC != 0 ? PUB_RC : SUB_RC ))
