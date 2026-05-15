#!/usr/bin/env bash
set -euo pipefail

LOOPS="${1:-1000}"
mkdir -p logs/perf examples/logs

if ! command -v perf >/dev/null 2>&1; then
  echo "[profile_preprocess_perf] perf not found. Install linux-tools for your kernel, or skip this optional profile."
  exit 0
fi

cd examples
make bin/13_preprocess_benchmark_compare
cd ..

perf stat -o logs/perf/preprocess_perf_stat.txt \
  ./examples/bin/13_preprocess_benchmark_compare "$LOOPS" \
  > logs/perf/preprocess_perf_run.log 2>&1 || true

echo "[profile_preprocess_perf] stat=logs/perf/preprocess_perf_stat.txt"
echo "[profile_preprocess_perf] run_log=logs/perf/preprocess_perf_run.log"