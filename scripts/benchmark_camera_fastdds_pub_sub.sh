#!/usr/bin/env bash
set -euo pipefail
FRAMES="${1:-60}"
WIDTH="${2:-640}"
HEIGHT="${3:-480}"
FPS="${4:-30}"
FORMAT="${5:-yuyv}"
TOPIC="${6:-avm/camera/synthetic_raw}"
DEPTH="${7:-1024}"
REPEAT="${8:-10}"
WARMUP="${9:-10}"
START_SEQ=$((WARMUP + 1))
OUT_DIR="logs/benchmark_v2_4_fastdds_camera_synthetic"
mkdir -p "$OUT_DIR"

./build/camera_fastdds_sub \
  --topic "$TOPIC" \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --format "$FORMAT" \
  --frames "$FRAMES" \
  --start-seq "$START_SEQ" \
  --depth "$DEPTH" \
  --max-payload 2097152 \
  --timeout-ms 120000 \
  --reliable \
  --output "$OUT_DIR/camera_fastdds_sub.csv" \
  --allow-unavailable > "$OUT_DIR/camera_fastdds_sub.log" 2>&1 &
SUB_PID=$!
sleep 5

PUB_RC=0
./build/camera_fastdds_pub \
  --source synthetic \
  --topic "$TOPIC" \
  --width "$WIDTH" \
  --height "$HEIGHT" \
  --fps "$FPS" \
  --format "$FORMAT" \
  --frames "$FRAMES" \
  --depth "$DEPTH" \
  --max-payload 2097152 \
  --reliable \
  --startup-ms 5000 \
  --linger-ms 5000 \
  --warmup-frames "$WARMUP" \
  --repeat "$REPEAT" \
  --inter-repeat-us 1000 \
  --output "$OUT_DIR/camera_fastdds_pub.csv" \
  --allow-unavailable > "$OUT_DIR/camera_fastdds_pub.log" 2>&1 || PUB_RC=$?
SUB_RC=0
wait "$SUB_PID" || SUB_RC=$?

cat "$OUT_DIR/camera_fastdds_pub.csv"
cat "$OUT_DIR/camera_fastdds_sub.csv"
exit $(( PUB_RC != 0 ? PUB_RC : SUB_RC ))
