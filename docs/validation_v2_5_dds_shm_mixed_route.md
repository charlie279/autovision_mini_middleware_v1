# V2.5 DDS + SHM Mixed Route Validation

## 1. 目标

V2.5 将 V2.4 的 raw frame over DDS 验证链路升级为：

```text
Camera/Synthetic frame raw payload -> POSIX SHM FramePool
SharedFrameDescriptor metadata -> FastDDS/RTPS topic
Subscriber metadata -> SHM payload read -> CRC/size/seq/latency validation
```

此设计用于证明项目具备自动驾驶中间件常见的大帧数据面优化意识：DDS 适合发布控制/元数据，raw camera payload 更适合通过 SHM/DMABUF 一类共享数据面传递。

## 2. 新增能力

| 能力 | 文件/模块 | 状态 |
|---|---|---|
| SharedFrameDescriptor 元数据结构 | `include/dds_shm_frame_meta.hpp` | 已实现 |
| descriptor 序列化/反序列化 | `src/dds_shm_frame_meta.cpp` | 已实现 |
| metadata envelope codec | `make_shared_frame_descriptor_envelope` / `parse_shared_frame_descriptor_envelope` | 已实现 |
| Camera/Synthetic -> SHM payload writer | `camera_dds_shm_pub` | 已实现 |
| FastDDS metadata publish | `camera_dds_shm_pub` | 已实现，等待 FastDDS 环境实测 |
| FastDDS metadata subscribe | `camera_dds_shm_sub` | 已实现，等待 FastDDS 环境实测 |
| SHM payload CRC/size/seq 验证 | `camera_dds_shm_sub` | 已实现 |
| dependency-free descriptor example | `25_dds_shm_frame_meta` | 已验证 |
| dependency-free NOT_COMPILED fallback | `benchmark_camera_dds_shm.sh` | 已验证 |

## 3. 当前容器已验证

```text
./scripts/build.sh：PASS
./scripts/run_all_vm.sh：PASS，final_status=NORMAL，media/preprocess/npu=120/120/120
examples/23_fastdds_packet_codec：PASS
examples/24_camera_fastdds_packet：PASS
examples/25_dds_shm_frame_meta：PASS，metadata_bytes=208，raw_payload_bytes=614400，size_errors=0，payload_errors=0
benchmark_camera_dds_shm.sh dependency-free fallback：pub/sub 均输出 NOT_COMPILED
```

## 4. 当前容器具体验证输出

### 4.1 主链路回归

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

### 4.2 examples 23/24/25

```text
[23_fastdds_packet_codec] fastdds_status=not_compiled: install FastDDS/Fast-RTPS 2.12.x and rebuild with -DAVM_ENABLE_FASTDDS=ON
[23_fastdds_packet_codec] encoded_bytes=160096 kind=lidar seq=7 payload=160000 decode=OK validate=OK result=PASS
[24_camera_fastdds_packet] fastdds_status=not_compiled: install FastDDS/Fast-RTPS 2.12.x and rebuild with -DAVM_ENABLE_FASTDDS=ON
[24_camera_fastdds_packet] yuyv_payload=614400 yuyv_packet=614496 rgb_payload=921600 rgb_packet=921696 v24_qos_yuyv=OK v24_qos_rgb=OK old_512k_qos=EXPECTED_FAIL result=PASS
[25_dds_shm_frame_meta] metadata_bytes=208 raw_payload_bytes=614400 size_errors=0 payload_errors=0 result=PASS
```

### 4.3 V2.5 dependency-free fallback

```text
role,compiled,source,topic,shm_name,format,width,height,frames_requested,warmup_frames,repeat,pool_count,buffer_size,metadata_bytes,raw_payload_bytes,tx_attempts,sent,shm_write_errors,metadata_errors,payload_errors,status
pub,false,synthetic,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,5,10,1,64,614400,0,0,0,0,0,0,0,NOT_COMPILED
role,compiled,topic,shm_name,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,metadata_errors,shm_read_errors,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status
sub,false,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,5,11,0,0,0,0,0,0,0,120000,0,0,NOT_COMPILED
```

## 5. 等待用户 VMware / FastDDS 环境验证

```text
1. FastDDS ON 编译：./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
2. synthetic 300 帧 DDS+SHM 混合路由验证。
3. 真实 USB Camera 300 帧 DDS+SHM 混合路由验证。
4. 与 V2.4 raw DDS 方案的 latency/CPU/DDS payload size 对比。
5. 更长时间 soak test 与跨主机 metadata-only FastDDS 验证。
```

## 6. 验收标准

### 6.1 dependency-free 验收

| 验收项 | 标准 | 当前结果 |
|---|---|---|
| 默认编译 | `./scripts/build.sh` 通过 | 已通过 |
| 主链路回归 | `final_status=NORMAL`，`120/120/120` | 已通过 |
| example 25 | `metadata_bytes=208`，`size_errors=0`，`payload_errors=0`，`result=PASS` | 已通过 |
| 未安装 FastDDS | pub/sub 均输出 `NOT_COMPILED`，不伪造 PASS | 已通过 |

### 6.2 FastDDS ON 验收

| 验收项 | 标准 |
|---|---|
| synthetic 300 | `unique_received=300`、`lost=0`、`metadata_errors=0`、`shm_read_errors=0`、`size_errors=0`、`payload_errors=0`、`status=PASS` |
| real camera 300 | `unique_received=300`、`lost=0`、`payload_errors=0`、`status=PASS` |
| metadata size | `metadata_bytes` 约 208 bytes，显著低于 640×480 YUYV raw payload 614400 bytes |
| raw payload | 通过 SHM FramePool 读取，CRC 与 descriptor 中 `payload_crc32` 一致 |

## 7. 边界说明

```text
1. V2.5 仍是同机双进程 DDS+SHM 验证，不宣称跨主机 SHM 共享。
2. V2.5 使用 FastDDS 传 AutoVision TransportEnvelope 中的 descriptor payload，不是完整 IDL typed DataWriter/DataReader。
3. V2.5 使用 POSIX SHM FramePool，不宣称 DMA-BUF zero-copy 主链路已经完成。
4. V2.5 不接真实 NPU/RKNN/AXera SDK，不改变 V2.4 的 NPU stub/runtime 抽象边界。
5. V2.5 的目标是验证 mixed-route 架构，不替代后续 V2.6 typed DDS、V2.7 soak test、V3.0 板端 DMA-BUF/NPU/编码器迁移。
```
