#!/usr/bin/env bash
set -euo pipefail

FRAMES="${1:-20}"
DEPTH="${2:-8}"
WIDTH_NOTE="CamMW-style Transport pattern benchmark: Transport -> Transmitter -> Rtps/Shm backend -> Dispatcher -> Receiver"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
mkdir -p logs/benchmark_v2_2

echo "[benchmark_transport_pattern] $WIDTH_NOTE"
./scripts/build.sh
./build/transport_pattern_demo --backend both --mode all --frames "$FRAMES" --depth "$DEPTH" --output logs/benchmark_v2_2/transport_pattern.csv

echo "[benchmark_transport_pattern] output=logs/benchmark_v2_2/transport_pattern.csv"
cat logs/benchmark_v2_2/transport_pattern.csv
