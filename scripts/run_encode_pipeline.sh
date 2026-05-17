
#!/usr/bin/env bash
# run_encode_pipeline.sh — media_node + encode_sink_node 端到端编码测试
# 用法：./scripts/run_encode_pipeline.sh soft h264 120
set -euo pipefail

BACKEND=${1:-soft}
CODEC=${2:-h264}
FRAMES=${3:-120}
WIDTH=${4:-640}
HEIGHT=${5:-480}
OUTPUT="logs/encode_output.${CODEC}"
CSV="logs/encode_benchmark.csv"

mkdir -p logs

echo "[run_encode_pipeline] backend=${BACKEND} codec=${CODEC} frames=${FRAMES} width=${WIDTH} height=${HEIGHT}"

./scripts/prepare_input.sh
./scripts/clean_ipc.sh

./build/media_node \
    --source file --input assets/input_640x480_rgb.raw \
    --width "${WIDTH}" --height "${HEIGHT}" --fps 30 --frames "${FRAMES}" \
    > logs/encode_media.log 2>&1 &
MEDIA_PID=$!

sleep 0.5

./build/encode_sink_node \
    --backend "${BACKEND}" \
    --codec "${CODEC}" \
    --output "${OUTPUT}" \
    --frames "${FRAMES}" \
    --width "${WIDTH}" --height "${HEIGHT}" \
    --benchmark --csv "${CSV}" \
    | tee logs/encode_sink.log

wait "${MEDIA_PID}" 2>/dev/null || true

echo "[run_encode_pipeline] output=${OUTPUT}"
ls -lh "${OUTPUT}"
echo "[run_encode_pipeline] csv=${CSV}"
cat "${CSV}"

