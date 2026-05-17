# V2.2 新增代码放置、运行命令与 Git 步骤

## 1. 解压覆盖

```bash
mkdir -p ~/projects
cd ~/projects
unzip -o autovision_mini_middleware_v2_2_transport_pattern_source.zip
cd autovision_mini_middleware_v1
chmod +x scripts/*.sh
```

## 2. 依赖

```bash
sudo apt update
sudo apt install -y build-essential cmake git python3 ffmpeg v4l-utils
```

## 3. 编译

```bash
./scripts/build.sh
```

## 4. V1.9/V2.0 主链路回归

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt

./scripts/run_encode_pipeline.sh soft h264 5
ffprobe -v error -show_entries stream=codec_name,width,height -of default=noprint_wrappers=1 logs/encode_output.h264
```

## 5. V2.1 四链路 payload 验证

```bash
./build/transport_four_links_demo --mode all --frames 5 --output logs/transport_four_links_v22.csv
cat logs/transport_four_links_v22.csv
```

## 6. V2.2 Transport pattern 验证

```bash
./scripts/benchmark_transport_pattern.sh 5 8
cat logs/benchmark_v2_2/transport_pattern.csv
```

## 7. examples 验证

```bash
cd examples
make bin/21_transport_four_links bin/22_transport_pattern
make run EXAMPLE=21_transport_four_links
make run EXAMPLE=22_transport_pattern
```

## 8. Git 分阶段提交建议

```bash
git checkout v2.1-transport-bridge
git pull
git checkout -b v2.2-transport-pattern

git add include/transport_endpoint.hpp src/transport_endpoint.cpp
git commit -m "v2.2-step1: add reference transport-style Transport endpoint abstraction"

git add src/transport_pattern_demo.cpp scripts/benchmark_transport_pattern.sh CMakeLists.txt
git commit -m "v2.2-step2: add Transport pattern demo and benchmark script"

git add examples/cpp/22_transport_pattern.cpp examples/Makefile
git commit -m "v2.2-step3: add Transport pattern interface example"

git add docs/validation_v2_2_transport_pattern.md docs/V2_2_新增命令与Git步骤.md README.md
git commit -m "v2.2-step4: document Transport pattern validation"

git push -u origin v2.2-transport-pattern
```

## 9. 不应提交的运行产物

```text
build/
logs/
assets/
examples/bin/
examples/logs/
*.h264
*.h265
*.raw_input
compile_commands.json
```
