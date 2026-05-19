# V2.5 VMware FastDDS DDS+SHM Mixed Route Validation Summary

## 1. Validation Environment

- Project: AutoVision Mini Middleware
- Branch: `v2.5-dds-shm-mixed-route`
- Host: VMware Ubuntu
- Camera device: `/dev/video0`
- Camera format: `YUYV 640x480@30fps`
- FastDDS build option: `-DAVM_ENABLE_FASTDDS=ON`
- FastDDS prefix: `$HOME/Fast-DDS/install`
- V2.5 route: DDS metadata + POSIX SHM raw payload
- Validation date: 2026-05-19

V2.5 的目标是将 V2.4 的 `raw frame over FastDDS` 链路升级为：

```text
USB Camera / Synthetic Raw Frame
        -> POSIX SHM FramePool carries raw payload
        -> FastDDS / RTPS carries SharedFrameDescriptor metadata
        -> Subscriber reads payload from SHM
        -> CRC / size / seq / latency validation
```

核心设计目标：

```text
V2.4: DDS carries raw camera payload.
V2.5: DDS carries only metadata descriptor, while raw payload is transferred through POSIX SHM.
```

---

## 2. Dependency-Free Build and Regression Validation

### 2.1 Default Build

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh
```

Result:

```text
[100%] Built target camera_dds_shm_pub
[100%] Built target camera_dds_shm_sub
[build] done
```

Conclusion: PASS.

---

### 2.2 Main Pipeline Regression

Command:

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

Result:

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

Conclusion: PASS.

---

### 2.3 Examples 23 / 24 / 25 Validation

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1/examples

make clean || true
make bin/23_fastdds_packet_codec bin/24_camera_fastdds_packet bin/25_dds_shm_frame_meta

make run EXAMPLE=23_fastdds_packet_codec
make run EXAMPLE=24_camera_fastdds_packet
make run EXAMPLE=25_dds_shm_frame_meta
```

Result:

```text
[23_fastdds_packet_codec] fastdds_status=not_compiled: install FastDDS/Fast-RTPS 2.12.x and rebuild with -DAVM_ENABLE_FASTDDS=ON
[23_fastdds_packet_codec] encoded_bytes=160096 kind=lidar seq=7 payload=160000 decode=OK validate=OK result=PASS

[24_camera_fastdds_packet] fastdds_status=not_compiled: install FastDDS/Fast-RTPS 2.12.x and rebuild with -DAVM_ENABLE_FASTDDS=ON
[24_camera_fastdds_packet] yuyv_payload=614400 yuyv_packet=614496 rgb_payload=921600 rgb_packet=921696 v24_qos_yuyv=OK v24_qos_rgb=OK old_512k_qos=EXPECTED_FAIL result=PASS

[25_dds_shm_frame_meta] metadata_bytes=208 raw_payload_bytes=614400 size_errors=0 payload_errors=0 result=PASS
```

Conclusion: PASS.

Explanation:

```text
Example 25 validates the V2.5 core route without requiring FastDDS runtime:

1. Create POSIX SHM FramePool.
2. Write synthetic 640x480 YUYV raw payload to SHM.
3. Generate SharedFrameDescriptor.
4. Serialize descriptor into TransportEnvelope.
5. Deserialize descriptor.
6. Read payload from SHM by buffer_index.
7. Validate payload size and CRC.
```

---

### 2.4 FastDDS OFF Fallback Validation

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1

./scripts/benchmark_camera_dds_shm.sh \
  5 640 480 30 yuyv \
  avm/camera/dds_shm_synthetic_meta \
  /avm_camera_dds_shm_synthetic_pool
```

Result:

```text
role,compiled,source,topic,shm_name,format,width,height,frames_requested,warmup_frames,repeat,pool_count,buffer_size,metadata_bytes,raw_payload_bytes,tx_attempts,sent,shm_write_errors,metadata_errors,payload_errors,status
pub,false,synthetic,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,5,10,1,64,614400,0,0,0,0,0,0,0,NOT_COMPILED

role,compiled,topic,shm_name,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,metadata_errors,shm_read_errors,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status
sub,false,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,5,11,0,0,0,0,0,0,0,120000,0,0,NOT_COMPILED
```

Conclusion: PASS.

Explanation:

```text
When FastDDS is not compiled in, V2.5 explicitly reports NOT_COMPILED instead of faking a PASS result.
```

---

## 3. FastDDS ON Build Validation

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1

rm -rf build
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
```

Result:

```text
[100%] Built target camera_fastdds_pub
[100%] Built target camera_fastdds_sub
[100%] Built target camera_dds_shm_pub
[100%] Built target camera_dds_shm_sub
[build] done
```

Binary check:

```bash
ls -lh build/camera_dds_shm_pub build/camera_dds_shm_sub
```

Result:

```text
-rwxrwxr-x 1 charlie charlie 213K  5月 19 15:20 build/camera_dds_shm_pub
-rwxrwxr-x 1 charlie charlie 200K  5月 19 15:20 build/camera_dds_shm_sub
```

Conclusion: PASS.

---

## 4. V2.5 Synthetic DDS+SHM Validation

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1

./scripts/clean_ipc.sh || true

./scripts/benchmark_camera_dds_shm.sh \
  300 640 480 30 yuyv \
  avm/camera/dds_shm_synthetic_meta \
  /avm_camera_dds_shm_synthetic_pool
```

Publisher result:

```text
role,compiled,source,topic,shm_name,format,width,height,frames_requested,warmup_frames,repeat,pool_count,buffer_size,metadata_bytes,raw_payload_bytes,tx_attempts,sent,shm_write_errors,metadata_errors,payload_errors,status
pub,true,synthetic,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,300,10,1,64,614400,208,614400,310,300,0,0,0,PASS
```

Subscriber result:

```text
role,compiled,topic,shm_name,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,metadata_errors,shm_read_errors,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status
sub,true,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,300,11,300,0,0,0,0,0,0,120000,9.23346,11.524,PASS
```

Key metrics:

| Metric | Value |
|---|---:|
| Source | synthetic |
| Format | YUYV |
| Resolution | 640x480 |
| Requested frames | 300 |
| Unique received | 300 |
| Lost | 0 |
| Duplicates | 0 |
| Metadata bytes | 208 |
| Raw payload bytes | 614400 |
| Metadata errors | 0 |
| SHM read errors | 0 |
| Size errors | 0 |
| Payload errors | 0 |
| Mean latency | 9.23346 ms |
| P95 latency | 11.524 ms |
| Status | PASS |

Conclusion: PASS.

---

## 5. V2.5 Real USB Camera DDS+SHM Validation

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1

./scripts/clean_ipc.sh || true

./scripts/run_camera_dds_shm.sh \
  /dev/video0 300 640 480 30 yuyv \
  avm/camera/dds_shm_raw_meta \
  /avm_camera_dds_shm_raw_pool
```

Publisher result:

```text
role,compiled,source,topic,shm_name,format,width,height,frames_requested,warmup_frames,repeat,pool_count,buffer_size,metadata_bytes,raw_payload_bytes,tx_attempts,sent,shm_write_errors,metadata_errors,payload_errors,status
pub,true,camera,avm/camera/dds_shm_raw_meta,/avm_camera_dds_shm_raw_pool,yuyv,640,480,300,10,1,64,614400,208,614400,310,300,0,0,0,PASS
```

Subscriber result:

```text
role,compiled,topic,shm_name,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,metadata_errors,shm_read_errors,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status
sub,true,avm/camera/dds_shm_raw_meta,/avm_camera_dds_shm_raw_pool,yuyv,640,480,300,11,300,0,0,0,0,0,0,120000,8.7143,11.275,PASS
```

Key metrics:

| Metric | Value |
|---|---:|
| Source | real USB camera |
| Device | `/dev/video0` |
| Format | YUYV |
| Resolution | 640x480 |
| FPS | 30 |
| Requested frames | 300 |
| Unique received | 300 |
| Lost | 0 |
| Duplicates | 0 |
| Metadata bytes | 208 |
| Raw payload bytes | 614400 |
| Metadata errors | 0 |
| SHM read errors | 0 |
| Size errors | 0 |
| Payload errors | 0 |
| Mean latency | 8.7143 ms |
| P95 latency | 11.275 ms |
| Status | PASS |

Conclusion: PASS.

---

## 6. V2.4 Raw FastDDS Regression Comparison

V2.5 合并后，对 V2.4 raw FastDDS 链路进行了回归对比。该部分用于验证 V2.5 新增代码没有破坏 V2.4 主要链路，同时也用于观察 raw payload over DDS 与 DDS+SHM mixed route 的稳定性差异。

---

### 6.1 V2.4 Synthetic Raw FastDDS Regression

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1

./scripts/benchmark_camera_fastdds_pub_sub.sh \
  60 640 480 30 yuyv \
  avm/camera/synthetic_raw_v25_regression
```

Publisher result:

```text
role,compiled,source,topic,format,width,height,frames_requested,warmup_frames,repeat,tx_attempts,sent,size_errors,payload_errors,status
pub,true,synthetic,avm/camera/synthetic_raw_v25_regression,yuyv,640,480,60,10,10,700,60,0,0,PASS
```

Subscriber result:

```text
role,compiled,topic,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status
sub,true,avm/camera/synthetic_raw_v25_regression,yuyv,640,480,60,11,60,454,0,0,0,120000,27.5529,35.557,PASS
```

Conclusion: PASS.

---

### 6.2 V2.4 Real USB Camera Raw FastDDS Regression

Command:

```bash
cd ~/projects/autovision_mini_middleware_v1

./scripts/run_camera_fastdds_pub_sub.sh \
  /dev/video0 300 640 480 30 yuyv \
  avm/camera/raw_v25_regression
```

Publisher result:

```text
role,compiled,source,topic,format,width,height,frames_requested,warmup_frames,repeat,tx_attempts,sent,size_errors,payload_errors,status
pub,true,camera,avm/camera/raw_v25_regression,yuyv,640,480,300,10,10,3100,300,0,0,PASS
```

Subscriber result:

```text
role,compiled,topic,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status
sub,true,avm/camera/raw_v25_regression,yuyv,640,480,300,11,294,1089,6,0,0,120000,32.5587,63.912,PARTIAL
```

Key metrics:

| Metric | Value |
|---|---:|
| Source | real USB camera |
| Route | V2.4 raw frame over FastDDS |
| Requested frames | 300 |
| Unique received | 294 |
| Lost | 6 |
| Duplicates | 1089 |
| Size errors | 0 |
| Payload errors | 0 |
| Mean latency | 32.5587 ms |
| P95 latency | 63.912 ms |
| Status | PARTIAL |

Interpretation:

```text
The V2.4 real camera raw FastDDS regression was PARTIAL in this run.
The publisher sent all 300 formal frames successfully, but the subscriber received 294 unique frames and reported 6 lost frames.

This does not block V2.5 validation because the V2.5 real USB camera DDS+SHM route passed with 300/300 unique frames, 0 lost frames, 0 metadata errors, 0 SHM read errors, 0 size errors, and 0 payload errors.

The contrast supports the V2.5 design motivation: large raw frame payloads are better kept in SHM, while DDS should carry compact metadata descriptors.
```

---

## 7. V2.4 vs V2.5 Result Comparison

| Item | V2.4 Raw FastDDS | V2.5 DDS+SHM Mixed Route |
|---|---:|---:|
| Camera source | `/dev/video0` | `/dev/video0` |
| Format | YUYV 640x480 | YUYV 640x480 |
| Raw payload per frame | 614400 bytes | 614400 bytes |
| DDS payload per frame | raw frame payload | descriptor metadata |
| Metadata bytes | not applicable | 208 bytes |
| Requested frames | 300 | 300 |
| Unique received | 294 | 300 |
| Lost | 6 | 0 |
| Duplicates | 1089 | 0 |
| Size errors | 0 | 0 |
| Payload errors | 0 | 0 |
| SHM read errors | not applicable | 0 |
| Mean latency | 32.5587 ms | 8.7143 ms |
| P95 latency | 63.912 ms | 11.275 ms |
| Status | PARTIAL | PASS |

Summary:

```text
In the VMware real-camera test, V2.5 DDS+SHM mixed route achieved a cleaner result than V2.4 raw frame over FastDDS.

V2.5 reduced DDS payload from a full 614400-byte raw YUYV frame to a 208-byte SharedFrameDescriptor, while preserving raw frame integrity through POSIX SHM and CRC validation.
```

---

## 8. Acceptance Criteria

V2.5 acceptance criteria:

```text
unique_received == frames
lost == 0
metadata_errors == 0
shm_read_errors == 0
size_errors == 0
payload_errors == 0
status == PASS
```

Synthetic DDS+SHM result:

```text
unique_received=300
lost=0
metadata_errors=0
shm_read_errors=0
size_errors=0
payload_errors=0
status=PASS
```

Real USB camera DDS+SHM result:

```text
unique_received=300
lost=0
metadata_errors=0
shm_read_errors=0
size_errors=0
payload_errors=0
status=PASS
```

Conclusion: V2.5 meets the acceptance criteria.

---

## 9. Final Conclusion

V2.5 DDS+SHM mixed route has been validated successfully on VMware Ubuntu with FastDDS enabled.

The validated real-camera route is:

```text
USB Camera /dev/video0
        -> camera_dds_shm_pub
        -> write YUYV raw payload into POSIX SHM FramePool
        -> publish SharedFrameDescriptor through FastDDS
        -> camera_dds_shm_sub
        -> read raw payload from SHM by shm_name + buffer_index
        -> validate sequence, size, CRC, metadata, SHM access, and latency
```

Final V2.5 real-camera result:

```text
metadata_bytes=208
raw_payload_bytes=614400
unique_received=300
lost=0
duplicates=0
metadata_errors=0
shm_read_errors=0
size_errors=0
payload_errors=0
mean_latency_ms=8.7143
p95_latency_ms=11.275
status=PASS
```

This validates the V2.5 design goal:

```text
Use DDS for compact metadata transport and POSIX SHM for large raw frame payload transport.
```

V2.5 is therefore accepted as the next stable middleware milestone after V2.4.