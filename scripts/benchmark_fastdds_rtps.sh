#!/usr/bin/env bash
set -euo pipefail
FRAMES="${1:-5}"
DEPTH="${2:-8}"
OUT_DIR="logs/benchmark_v2_3_fastdds"
mkdir -p "$OUT_DIR"
./build/fastdds_rtps_loopback_demo \
  --frames "$FRAMES" \
  --depth "$DEPTH" \
  --topic "avm/v2_3/fastdds/raw" \
  --output "$OUT_DIR/fastdds_rtps.csv" \
  --allow-unavailable
cat "$OUT_DIR/fastdds_rtps.csv"

