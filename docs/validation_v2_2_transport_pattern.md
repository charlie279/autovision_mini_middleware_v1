# AutoVision Mini Middleware V2.2 Transport Pattern Validation

## 版本定位

V2.2 在 V2.1 reference transport 四链路 payload 校验基础上，新增与通信设计图一致的通信抽象：

```text
Transport
  ├── createTransmitter()
  │     └── Transmitter
  │           ├── RtpsTransmitter
  │           └── ShmTransmitter
  └── createReceiver()
        └── Receiver
              ├── RtpsReceiver + RtpsDispatcher
              └── ShmReceiver + ShmDispatcher
```

当前 generic Linux/VMware 环境不引入 FastDDS 外部依赖。`RtpsTransmitter/RtpsReceiver` 是 RTPS-style local backend，用于验证抽象边界、消息校验、大 payload、sequence、CRC、drop/lost 统计和测试体系；真实 FastDDS/RTPS backend 留到后续 V2.3 与 reference transport 进一步融合。

## 已验证命令

### 1. 主工程编译

```bash
./scripts/build.sh
```

结果：通过，新增 `avm_transport`、`transport_four_links_demo`、`transport_pattern_demo` 均构建成功。

### 2. 文件输入链路回归

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

结果：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

### 3. V2.0 H.264 编码链路回归

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

### 4. V2.1 四链路 payload 回归

```bash
./build/transport_four_links_demo --mode all --frames 5 --output logs/transport_four_links_v22.csv
```

结果：Test / Encoded-small / Lidar-small / Raw-small / Encoded / Lidar / Raw 全部 PASS；Raw full payload 为 307200 bytes，Lidar full payload 为 160000 bytes。

### 5. V2.2 Transport pattern 双后端验证

```bash
./scripts/benchmark_transport_pattern.sh 5 8
```

结果：`rtps` 与 `shm` 两类 backend 均完成 Test / Encoded-small / Lidar-small / Raw-small / Encoded / Lidar / Raw 全链路验证，所有场景 `sent=received`，`dropped=0`，`lost=0`，`size_errors=0`，`payload_errors=0`，结果均为 `PASS`。

### 6. examples 21/22

```bash
cd examples
make bin/21_transport_four_links bin/22_transport_pattern
make run EXAMPLE=21_transport_four_links
make run EXAMPLE=22_transport_pattern
```

结果：

```text
[21_transport_four_links] kind=raw seq=1 bytes=307200 size_error=0 payload_error=0 result=PASS
[22_transport_pattern] backend=rtps tx=RtpsTransmitter(local_rtps_style) rx=RtpsReceiver(local_rtps_style) kind=lidar seq=0 bytes=160000 size_error=0 payload_error=0 lost=0 result=PASS
```

## 等待验证

```text
1. 真实 FastDDS/RTPS backend 尚未接入 AutoVision 主工程。
2. reference transport 原仓库 FastDDS publisher/subscriber 与 AutoVision Transport API 的二进制兼容桥接尚未验证。
3. 跨进程 RTPS/SHM 双模式真实 pub/sub 尚未替代当前 local backend。
4. VMware 下 USB Camera RGB/YUYV 在本次容器无 /dev/video0，仍以既有工作日志为准，用户侧可继续复测。
5. OPi5+/RK3588 板端 MPP/V4L2 M2M/RKNN/RGA/DMA-BUF 仍等待开发板验证。
```

