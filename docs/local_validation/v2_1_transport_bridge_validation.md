# V2.1 reference transport Transport Bridge Local Validation

## 基线

- AutoVision 基准：V2.0 Video Encode 代码结构。
- reference transport 迁移点：四链路 TestMsg / RawFrame / EncodedFrame / LidarFrame，大 payload msg_size/depth/payload 校验。
- 本地实现：dependency-free local pub/sub backend，不依赖 FastDDS。

## 1. 主工程编译

命令：

```bash
./scripts/build.sh
```

结果：通过。新增 target：

```text
avm_transport
transport_four_links_demo
```

## 2. V2.0 文件链路回归

命令：

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

结果：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## 3. V2.0 H.264 编码回归

命令：

```bash
./scripts/run_encode_pipeline.sh soft h264 5
ffprobe -v error -show_entries stream=codec_name,width,height -of default=noprint_wrappers=1 logs/encode_output.h264
```

结果：

```text
codec_name=h264
width=640
height=480
```

## 4. V2.1 reference transport-style 四链路验证

命令：

```bash
./scripts/benchmark_transport_four_links.sh 10
cat logs/benchmark_v2_1_transport/four_links.csv
```

结果摘要：

```text
test: payload_bytes=30, PASS
encoded-small: payload_bytes=4800, PASS
lidar-small: payload_bytes=16384, PASS
raw-small: payload_bytes=19200, PASS
encoded: payload_bytes=76800, PASS
lidar: payload_bytes=160000, PASS
raw: payload_bytes=307200, PASS
```

所有链路均满足：

```text
published=frames
received=frames
dropped=0
lost=0
size_errors=0
payload_errors=0
pass=PASS
```

## 5. examples 级验证

命令：

```bash
cd examples
make bin/21_transport_four_links
make run EXAMPLE=21_transport_four_links
```

结果：

```text
[21_transport_four_links] kind=raw seq=1 bytes=307200 size_error=0 payload_error=0 result=PASS
```

## 6. 当前补充说明

```text
1. 主工程 CMake 已完整编译通过。
2. 新增 example 21 已通过 Makefile 单独编译和运行验证。
3. 原 V2.0 example 20 可通过等价 g++ 命令单独编译；完整 make all 在当前受限执行窗口中可能因编译时间被中断，不属于代码语法错误。
```

## 7. 当前等待验证

```text
1. 真实 FastDDS backend 尚未接入 AutoVision。reference transport 仓库本身的 FastDDS 大 payload 修复结果不能直接等价为 AutoVision 内部 DDS 已接入。
2. V2.1 仅验证 local pub/sub backend 的 payload 安全、队列深度与四链路 schema。
3. 跨进程 DDS discovery、RTPS writer/reader history、UDP/TCP/SHM DDS transport、ROS2 互操作仍需后续版本验证。
4. VMware 环境没有真实 /dev/video0 时，Camera pipeline 仍需在用户 VMware 或开发板侧复测。
5. MPP/V4L2 M2M/RKNN/RGA/DMA-BUF 仍保持 V2.0 边界说明：接口预留，不宣称板端真实闭环。
```

