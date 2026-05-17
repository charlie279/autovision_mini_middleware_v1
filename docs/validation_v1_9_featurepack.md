# AutoVision V1.9 Feature Pack Validation Report

Validated in the current Linux sandbox:

- CMake main build: PASS.
- File-input end-to-end pipeline: PASS, final status `NORMAL`, 120/120/120 frames.
- `algorithm_stub_node`: PASS, generated `logs/detection_results.jsonl`.
- Existing V1.8 preprocess/FrameLease benchmark: PASS.
- New examples 15–19: PASS.
- Record/replay demo: PASS.
- CPU affinity/priority benchmark: PASS; non-root FIFO priority failure is expected and handled.

Waiting for user-side validation:

- Real `/dev/video0` USB Camera RGB path on VMware Ubuntu.
- Real `/dev/video0` USB Camera YUYV fused path on VMware Ubuntu.
- GStreamer camera baseline.
- perf availability on the user’s kernel.
- Orange Pi 5 Plus / ARM64 build and board-side camera/NPU/RGA validation.

