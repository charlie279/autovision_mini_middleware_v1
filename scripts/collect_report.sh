#!/usr/bin/env bash
set -euo pipefail
mkdir -p docs
{
  echo "# V1 Runtime Report"
  echo
  echo "## Final Status"
  cat logs/final_status.txt 2>/dev/null || echo "(not found)"
  echo
  echo "## Media Log Tail"
  tail -n 20 logs/media_node.log 2>/dev/null || echo "(not found)"
  echo
  echo "## Preprocess Log Tail"
  tail -n 20 logs/preprocess_node.log 2>/dev/null || echo "(not found)"
  echo
  echo "## NPU Log Tail"
  tail -n 20 logs/npu_stub_node.log 2>/dev/null || echo "(not found)"
  echo
  echo "## Safety Log Tail"
  tail -n 20 logs/safety_monitor.log 2>/dev/null || echo "(not found)"
} > docs/performance_report.md
echo "[collect_report] written to docs/performance_report.md"
