#!/usr/bin/env bash
set -euo pipefail

LOOPS="${1:-100}"
mkdir -p logs/benchmark examples/logs

cd examples
make clean >/dev/null 2>&1 || true
make bin/13_preprocess_benchmark_compare
cd ..

./examples/bin/13_preprocess_benchmark_compare "$LOOPS" | tee logs/benchmark/synthetic_compare.log
python3 scripts/visualize_perf_compare.py \
  --input examples/logs/preprocess_benchmark_compare.csv \
  --out-csv logs/benchmark/synthetic_perf_compare_summary.csv \
  --out-html logs/benchmark/synthetic_perf_compare.html

echo "[benchmark_v1_7_synthetic_compare] html=logs/benchmark/synthetic_perf_compare.html"