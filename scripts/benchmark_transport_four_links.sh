#!/usr/bin/env bash
set -euo pipefail

FRAMES="${1:-30}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

./scripts/build.sh >/dev/null
mkdir -p logs/benchmark_v2_1_transport

./build/transport_four_links_demo \
  --mode all \
  --frames "${FRAMES}" \
  --depth 8 \
  --max-payload 524288 \
  --output logs/benchmark_v2_1_transport/four_links.csv

echo "[benchmark_transport_four_links] output=logs/benchmark_v2_1_transport/four_links.csv"

