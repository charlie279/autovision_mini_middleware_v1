# AutoVision Mini Middleware V1.7 Linux Validation Report

验证环境：当前 ChatGPT Linux 沙箱，x86_64，GCC/CMake 环境。
验证时间：2026-05-15。

## 已验证项目

1. 主工程 `./scripts/build.sh`：通过。
2. V1 文件输入链路 `./scripts/run_all_vm.sh`：通过，最终状态为 `NORMAL`。
3. `examples/Makefile` 全量编译：通过。
4. `examples/bin/12_yuyv_fused_preprocess`：通过，合成 YUYV 前处理函数可运行。
5. `examples/bin/13_preprocess_benchmark_compare`：通过，生成 RGB vs YUYV 合成前处理对比 CSV。
6. `scripts/visualize_perf_compare.py`：通过，生成 HTML/SVG 可视化性能对比报告。
7. `scripts/benchmark_v1_7_synthetic_compare.sh`：通过，端到端生成 synthetic benchmark HTML。

## 当前环境未验证项目

1. `/dev/video0` USB UVC 摄像头实机链路：当前容器无真实摄像头设备，等待 VMware Ubuntu 或香橙派 5 Plus 验证。
2. 香橙派 5 Plus ARM64 编译与运行：等待板端环境验证。
3. 真实 RKNN Runtime：当前仍为 `rknn_stub`，等待后续 SDK 接入。

## 文件输入链路最终状态

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## 合成性能对比说明

本地 synthetic benchmark 只用于证明 RGB 与 YUYV 两条前处理函数、CSV 记录与 HTML 可视化链路可工作。真实摄像头端到端收益必须以 VMware/香橙派上 `scripts/benchmark_v1_7_camera_compare.sh` 输出为准。

V1.7 的确定性收益是 payload 带宽：640×480 RGB888 为 921,600 B/frame，YUYV raw 为 614,400 B/frame，理论减少 307,200 B/frame，即约 33.3%。CPU preprocess 时延在不同平台上可能不同，不能用 synthetic 结果替代实机 Camera pipeline 结果。
