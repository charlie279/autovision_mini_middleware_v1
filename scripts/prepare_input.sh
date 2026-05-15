#!/usr/bin/env bash
set -euo pipefail
mkdir -p assets logs logs/preprocess
python3 - <<'PY'
from pathlib import Path
width, height, frames = 640, 480, 120
path = Path("assets/input_640x480_rgb.raw")
frame_bytes = width * height * 3
expected = frame_bytes * frames
if path.exists() and path.stat().st_size == expected:
    print(f"[prepare_input] exists: {path} ({path.stat().st_size} bytes)")
    raise SystemExit(0)
with path.open("wb") as f:
    for t in range(frames):
        buf = bytearray(frame_bytes)
        for y in range(height):
            row = y * width * 3
            for x in range(width):
                i = row + x * 3
                buf[i + 0] = (x + t * 3) % 256
                buf[i + 1] = (y + t * 2) % 256
                buf[i + 2] = (x + y + t * 5) % 256
        f.write(buf)
print(f"[prepare_input] generated: {path} ({expected} bytes)")
PY