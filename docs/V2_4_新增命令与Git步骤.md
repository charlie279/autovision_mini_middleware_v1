# V2.4 Camera FastDDS Pub/Sub 新增命令与 Git 步骤（稳定化修订版）

## 1. 版本定位

V2.4 在 V2.3 FastDDS/RTPS adapter 基础上补齐双进程 camera publisher/subscriber 验证链路：

```text
camera_fastdds_pub  -> FastDDS/RTPS -> camera_fastdds_sub
```

本修订版针对 VMware 实测中出现的 `pub PASS / sub PARTIAL` 问题做了稳定化处理：startup 等待、warmup 帧、subscriber 起始序号、重复发送与去重、严格返回码、默认 reliable/depth=512。

## 2. 新增文件

```text
src/camera_fastdds_pub.cpp
src/camera_fastdds_sub.cpp
examples/cpp/24_camera_fastdds_packet.cpp
scripts/benchmark_camera_fastdds_pub_sub.sh
scripts/run_camera_fastdds_pub_sub.sh
docs/V2_4_新增命令与Git步骤.md
docs/validation_v2_4_camera_fastdds_pubsub.md
```

## 3. 修改文件

```text
CMakeLists.txt
README.md
examples/Makefile
examples/README.md
include/fastdds_rtps_transport.hpp
src/fastdds_rtps_loopback_demo.cpp
```

## 4. 默认环境验证

```bash
cd ~/projects/autovision_mini_middleware_v1
chmod +x scripts/*.sh
./scripts/build.sh
./scripts/run_all_vm.sh
cd examples
make bin/23_fastdds_packet_codec bin/24_camera_fastdds_packet
make run EXAMPLE=23_fastdds_packet_codec
make run EXAMPLE=24_camera_fastdds_packet
```

## 5. FastDDS 环境验证

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
./scripts/benchmark_camera_fastdds_pub_sub.sh 60 640 480 30 yuyv avm/camera/synthetic_raw
```

查看：

```bash
cat logs/benchmark_v2_4_fastdds_camera_synthetic/camera_fastdds_pub.csv
cat logs/benchmark_v2_4_fastdds_camera_synthetic/camera_fastdds_sub.csv
```

## 6. 真实 USB Camera 验证

```bash
./scripts/run_camera_fastdds_pub_sub.sh /dev/video0 60 640 480 30 yuyv avm/camera/raw
cat logs/benchmark_v2_4_fastdds_camera/camera_fastdds_pub.csv
cat logs/benchmark_v2_4_fastdds_camera/camera_fastdds_sub.csv
```

## 7. Git 提交建议

```bash
git checkout v2.3-fastdds-rtps-adapter
git pull origin v2.3-fastdds-rtps-adapter
git checkout -b v2.4-camera-fastdds-pubsub

git add include/fastdds_rtps_transport.hpp src/fastdds_rtps_loopback_demo.cpp CMakeLists.txt
git commit -m "v2.4-step1: extend FastDDS payload boundary for camera frames"

git add src/camera_fastdds_pub.cpp src/camera_fastdds_sub.cpp
git commit -m "v2.4-step2: add stable camera FastDDS pubsub demos"

git add scripts/benchmark_camera_fastdds_pub_sub.sh scripts/run_camera_fastdds_pub_sub.sh
git commit -m "v2.4-step3: add stable camera FastDDS pubsub scripts"

git add examples/cpp/24_camera_fastdds_packet.cpp examples/Makefile examples/README.md
git commit -m "v2.4-step4: add camera FastDDS packet example"

git add docs/V2_4_新增命令与Git步骤.md docs/validation_v2_4_camera_fastdds_pubsub.md README.md
git commit -m "v2.4-step5: document camera FastDDS pubsub validation"

git push -u origin v2.4-camera-fastdds-pubsub
```

## 8. 不应提交

```text
build/
logs/
assets/
examples/bin/
examples/logs/
*.h264
*.h265
compile_commands.json
```
