#!/usr/bin/env bash
set -euo pipefail
ITER=${1:-20000000}
mkdir -p logs/benchmark
cd examples
make bin/19_affinity_priority_benchmark >/dev/null
cd ..
{
  echo "case,cpu,priority,elapsed_line"
  echo -n "default,-1,0,"; ./examples/bin/19_affinity_priority_benchmark --iterations "$ITER" | tr ',' ';'
  echo -n "cpu0,0,0,"; ./examples/bin/19_affinity_priority_benchmark --cpu 0 --iterations "$ITER" | tr ',' ';'
  echo -n "cpu0_fifo10,0,10,"; ./examples/bin/19_affinity_priority_benchmark --cpu 0 --priority 10 --iterations "$ITER" | tr ',' ';'
} > logs/benchmark/affinity_priority_benchmark.csv
cat logs/benchmark/affinity_priority_benchmark.csv

