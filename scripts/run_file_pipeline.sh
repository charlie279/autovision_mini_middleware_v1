#!/usr/bin/env bash
set -euo pipefail
./scripts/prepare_input.sh
./build/media_node --source file --input assets/input_640x480_rgb.raw --frames 120
