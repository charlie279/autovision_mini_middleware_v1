# V2.1 reference transport Transport Bridge 新增命令与 Git 步骤

## 1. 版本定位

V2.1 以 AutoVision Mini Middleware V2.0 为基准，不破坏 V2.0 已有 media/preprocess/NPU/algorithm/encode/control/safety 主线。新增内容聚焦 reference transport v4.7.2 中最值得迁移到 AutoVision 的工程能力：大 payload 安全边界、QoS msg_size/depth、四链路 Test/Raw/Encoded/Lidar schema 与可复现验证。

本版本不把 FastDDS 作为 AutoVision 的强依赖；当前实现为 dependency-free local pub/sub backend，方便在 VMware Ubuntu 与普通 Linux 环境编译运行。FastDDS 真实 backend 放入后续 V2.2/V2.3。

## 2. 新增文件

```text
include/transport_message.hpp
include/local_pubsub_transport.hpp
src/transport_message.cpp
src/local_pubsub_transport.cpp
src/transport_four_links_demo.cpp
examples/cpp/21_transport_four_links.cpp
scripts/benchmark_transport_four_links.sh
docs/local_validation/v2_1_cammw_transport_bridge_validation.md
docs/V2_1_新增命令与Git步骤.md
```

## 3. 修改文件

```text
CMakeLists.txt
README.md
examples/Makefile
examples/README.md
```

## 4. 编译

```bash
cd ~/projects/autovision_mini_middleware_v1
chmod +x scripts/*.sh
./scripts/build.sh
```

## 5. 回归验证

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

预期：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## 6. V2.0 编码链路回归

```bash
./scripts/run_encode_pipeline.sh soft h264 5
ffprobe -v error -show_entries stream=codec_name,width,height -of default=noprint_wrappers=1 logs/encode_output.h264
```

预期：

```text
codec_name=h264
width=640
height=480
```

## 7. V2.1 四链路验证

```bash
./scripts/benchmark_transport_four_links.sh 30
cat logs/benchmark_v2_1_transport/four_links.csv
```

预期每一行 `pass=PASS`，并且：

```text
size_errors=0
payload_errors=0
dropped=0
lost=0
```

## 8. 单模式调试

```bash
./build/transport_four_links_demo --mode test --frames 30
./build/transport_four_links_demo --mode encoded-small --frames 30
./build/transport_four_links_demo --mode lidar-small --frames 30
./build/transport_four_links_demo --mode raw-small --frames 30
./build/transport_four_links_demo --mode encoded --frames 30
./build/transport_four_links_demo --mode lidar --frames 30
./build/transport_four_links_demo --mode raw --frames 30
```

## 9. examples 验证

```bash
cd examples
make bin/21_transport_four_links
make run EXAMPLE=21_transport_four_links
```

预期：

```text
[21_transport_four_links] kind=raw seq=1 bytes=307200 size_error=0 payload_error=0 result=PASS
```

## 10. Git 提交建议

```bash
git checkout v2.0-video-encode
git pull origin v2.0-video-encode
git checkout -b v2.1-cammw-transport-bridge

git add include/transport_message.hpp include/local_pubsub_transport.hpp \
        src/transport_message.cpp src/local_pubsub_transport.cpp \
        src/transport_four_links_demo.cpp CMakeLists.txt

git commit -m "v2.1-step1: add reference transport-style local transport schema and backend"

git add examples/cpp/21_transport_four_links.cpp examples/Makefile examples/README.md

git commit -m "v2.1-step2: add transport four-link example"

git add scripts/benchmark_transport_four_links.sh README.md \
        docs/V2_1_新增命令与Git步骤.md \
        docs/local_validation/v2_1_cammw_transport_bridge_validation.md

git commit -m "v2.1-step3: add four-link validation script and docs"

git push -u origin v2.1-cammw-transport-bridge
```

