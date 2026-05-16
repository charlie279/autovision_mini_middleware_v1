#!/usr/bin/env bash
# benchmark_encode.sh — V2.0 编码 benchmark
set -euo pipefail

LOOPS=${1:-60}
WIDTH=${2:-640}
HEIGHT=${3:-480}
LOG_DIR="logs/benchmark_v2_0"
mkdir -p "${LOG_DIR}" examples/logs

echo "[benchmark_encode] loops=${LOOPS} width=${WIDTH} height=${HEIGHT}"

make -C examples all
./examples/bin/20_video_encode_benchmark --loops "${LOOPS}" --width "${WIDTH}" --height "${HEIGHT}"

cp examples/logs/encode_benchmark.csv "${LOG_DIR}/encode_benchmark.csv"
echo "[benchmark_encode] done csv=${LOG_DIR}/encode_benchmark.csv"
