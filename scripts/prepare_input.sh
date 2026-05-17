#!/usr/bin/env bash
set -euo pipefail
mkdir -p assets logs logs/preprocess
python3 - <<'PY'
from pathlib import Path
width, height, frames = 640, 480, 120
path = Path('assets/input_640x480_rgb.raw')
frame_bytes = width * height * 3
expected = frame_bytes * frames
if path.exists() and path.stat().st_size == expected:
    print(f'[prepare_input] exists: {path} ({path.stat().st_size} bytes)')
    raise SystemExit(0)
# Fast deterministic RGB pattern. Avoid slow per-pixel Python loops so a clean checkout
# can finish input generation quickly in VM/CI environments.
row = bytearray(width * 3)
for x in range(width):
    i = x * 3
    row[i + 0] = x & 0xFF
    row[i + 1] = (255 - x) & 0xFF
    row[i + 2] = (x * 3) & 0xFF
frame = bytes(row) * height
with path.open('wb') as f:
    for _ in range(frames):
        f.write(frame)
print(f'[prepare_input] generated: {path} ({expected} bytes)')
PY

