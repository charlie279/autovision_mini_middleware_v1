# V2.5 DDS + SHM Mixed Route 新增命令与 Git 步骤

## 1. 版本定位

V2.5 在 V2.4 Camera FastDDS Pub/Sub fixed 分支基础上，补齐自动驾驶视觉中间件更常见的高性能大帧传输模式：

```text
Camera / Synthetic raw payload -> POSIX SHM FramePool
SharedFrameDescriptor metadata -> FastDDS/RTPS topic
Subscriber receives metadata -> reads raw payload from SHM -> validates size/CRC/seq/latency
```

V2.4 已经证明 raw camera frame 可以通过 FastDDS packet 直接传输；V2.5 的目标不是替代 V2.4，而是把架构升级为 `metadata over DDS + payload over SHM`，避免将 640×480 YUYV/RGB888 raw payload 持续压入 DDS 数据面。

## 2. 新增文件

```text
include/dds_shm_frame_meta.hpp
src/dds_shm_frame_meta.cpp
src/camera_dds_shm_pub.cpp
src/camera_dds_shm_sub.cpp
examples/cpp/25_dds_shm_frame_meta.cpp
scripts/benchmark_camera_dds_shm.sh
scripts/run_camera_dds_shm.sh
docs/V2_5_新增命令与Git步骤.md
docs/validation_v2_5_dds_shm_mixed_route.md
```

## 3. 修改文件

```text
CMakeLists.txt
README.md
examples/Makefile
examples/README.md
```

## 4. 默认环境验证

```bash
cd ~/projects/autovision_mini_middleware_v1
chmod +x scripts/*.sh
./scripts/build.sh
./scripts/run_all_vm.sh
cd examples
make bin/23_fastdds_packet_codec bin/24_camera_fastdds_packet bin/25_dds_shm_frame_meta
make run EXAMPLE=23_fastdds_packet_codec
make run EXAMPLE=24_camera_fastdds_packet
make run EXAMPLE=25_dds_shm_frame_meta
```

预期：

```text
final_status=NORMAL
[23_fastdds_packet_codec] ... result=PASS
[24_camera_fastdds_packet] ... result=PASS
[25_dds_shm_frame_meta] metadata_bytes=208 raw_payload_bytes=614400 size_errors=0 payload_errors=0 result=PASS
```

## 5. V2.5 dependency-free fallback 验证

未安装 FastDDS 时，V2.5 脚本不伪造通信成功，而是让 pub/sub 均输出 `NOT_COMPILED`：

```bash
./scripts/benchmark_camera_dds_shm.sh 5 640 480 30 yuyv avm/camera/dds_shm_synthetic_meta /avm_camera_dds_shm_synthetic_pool
```

预期：

```text
pub,false,...,NOT_COMPILED
sub,false,...,NOT_COMPILED
```

## 6. FastDDS synthetic 验证

安装 FastDDS/Fast-CDR 并重新编译后：

```bash
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
./scripts/benchmark_camera_dds_shm.sh 300 640 480 30 yuyv avm/camera/dds_shm_synthetic_meta /avm_camera_dds_shm_synthetic_pool
```

查看：

```bash
cat logs/benchmark_v2_5_dds_shm_synthetic/camera_dds_shm_pub.csv
cat logs/benchmark_v2_5_dds_shm_synthetic/camera_dds_shm_sub.csv
```

预期核心指标：

```text
pub,true,synthetic,...,metadata_bytes=208,raw_payload_bytes=614400,...,sent=300,...,PASS
sub,true,...,unique_received=300,lost=0,metadata_errors=0,shm_read_errors=0,size_errors=0,payload_errors=0,...,PASS
```

## 7. 真实 USB Camera 验证

```bash
./scripts/run_camera_dds_shm.sh /dev/video0 300 640 480 30 yuyv avm/camera/dds_shm_raw_meta /avm_camera_dds_shm_raw_pool
cat logs/benchmark_v2_5_dds_shm_camera/camera_dds_shm_pub.csv
cat logs/benchmark_v2_5_dds_shm_camera/camera_dds_shm_sub.csv
```

预期核心指标：

```text
pub,true,camera,...,sent=300,...,PASS
sub,true,...,unique_received=300,lost=0,payload_errors=0,...,PASS
```

## 8. Git 提交建议

```bash
git checkout v2.4-camera-fastdds-pubsub-fixed
git pull origin v2.4-camera-fastdds-pubsub-fixed
git checkout -b v2.5-dds-shm-mixed-route

git add include/dds_shm_frame_meta.hpp src/dds_shm_frame_meta.cpp CMakeLists.txt
git commit -m "v2.5-step1: add DDS SHM frame descriptor codec"

git add src/camera_dds_shm_pub.cpp src/camera_dds_shm_sub.cpp
git commit -m "v2.5-step2: add camera DDS metadata plus SHM payload pubsub demos"

git add scripts/benchmark_camera_dds_shm.sh scripts/run_camera_dds_shm.sh
git commit -m "v2.5-step3: add DDS SHM mixed-route benchmark scripts"

git add examples/cpp/25_dds_shm_frame_meta.cpp examples/Makefile examples/README.md
git commit -m "v2.5-step4: add DDS SHM descriptor example"

git add docs/V2_5_新增命令与Git步骤.md docs/validation_v2_5_dds_shm_mixed_route.md README.md
git commit -m "v2.5-step5: document DDS SHM mixed-route validation"

git push -u origin v2.5-dds-shm-mixed-route
```

## 9. 不应提交

```text
build/
logs/
assets/
examples/bin/
examples/logs/
*.h264
*.h265
*.raw
*.yuyv
compile_commands.json
```
