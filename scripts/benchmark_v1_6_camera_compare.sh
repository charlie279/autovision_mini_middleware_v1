#!/usr/bin/env bash
set -euo pipefail

DEV="${1:-/dev/video0}"
FRAMES="${2:-900}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
FPS="${5:-30}"
BACKEND="${6:-dummy}"
OUT_DIR="logs/benchmark_v1_6"

mkdir -p "$OUT_DIR"

run_once() {
  local mode="$1"
  echo "[benchmark_v1_6] running camera_output=$mode"
  ./scripts/run_camera_pipeline.sh "$DEV" "$FRAMES" "$WIDTH" "$HEIGHT" "$FPS" "$BACKEND" "$mode"
  mkdir -p "$OUT_DIR/$mode"
  cp -f logs/*_camera.log logs/final_status_camera.txt "$OUT_DIR/$mode/" 2>/dev/null || true
}

run_once rgb
run_once yuyv

cat > "$OUT_DIR/README.md" <<REPORT
# V1.6 Camera Benchmark Output

Command:

\`\`\`bash
./scripts/benchmark_v1_6_camera_compare.sh $DEV $FRAMES $WIDTH $HEIGHT $FPS $BACKEND
\`\`\`

Outputs:

- rgb/: V1.5-compatible RGB888 intermediate path
- yuyv/: V1.6 YUYV raw + fused preprocess path

Manual checks:

\`\`\`bash
grep -E "Preprocess|finished" $OUT_DIR/rgb/preprocess_camera.log
grep -E "Preprocess|finished" $OUT_DIR/yuyv/preprocess_camera.log
cat $OUT_DIR/rgb/final_status_camera.txt
cat $OUT_DIR/yuyv/final_status_camera.txt
\`\`\`
REPORT

echo "[benchmark_v1_6] done. Check $OUT_DIR"
