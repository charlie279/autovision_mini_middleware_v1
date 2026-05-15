#!/usr/bin/env bash
set -euo pipefail

DEV="${1:-/dev/video0}"
FRAMES="${2:-120}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
FPS="${5:-30}"
BACKEND="${6:-dummy}"

./scripts/run_camera_pipeline.sh "$DEV" "$FRAMES" "$WIDTH" "$HEIGHT" "$FPS" "$BACKEND" yuyv
