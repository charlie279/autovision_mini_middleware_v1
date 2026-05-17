#!/usr/bin/env bash
set -euo pipefail

LOOPS="${1:-120}"
mkdir -p logs/benchmark examples/logs

cd examples
make clean >/dev/null 2>&1 || true
make bin/13_preprocess_benchmark_compare bin/14_frame_lease_pool
cd ..

./examples/bin/13_preprocess_benchmark_compare "$LOOPS" | tee logs/benchmark/v1_8_preprocess_compare.log
./examples/bin/14_frame_lease_pool | tee logs/benchmark/v1_8_frame_lease.log

python3 scripts/visualize_perf_compare.py \
  --input examples/logs/preprocess_benchmark_compare.csv \
  --out-csv logs/benchmark/v1_8_preprocess_compare_summary.csv \
  --out-html logs/benchmark/v1_8_preprocess_compare.html

cp examples/logs/preprocess_benchmark_compare.csv logs/benchmark/v1_8_preprocess_compare_raw.csv
cp examples/logs/frame_lease_pool.csv logs/benchmark/v1_8_frame_lease_pool.csv

echo "[benchmark_v1_8_vm_perf_opt] summary=logs/benchmark/v1_8_preprocess_compare_summary.csv"
echo "[benchmark_v1_8_vm_perf_opt] html=logs/benchmark/v1_8_preprocess_compare.html"
echo "[benchmark_v1_8_vm_perf_opt] lease_csv=logs/benchmark/v1_8_frame_lease_pool.csv"
