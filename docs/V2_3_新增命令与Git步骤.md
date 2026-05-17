# V2.3 FastDDS/RTPS 新增命令与 Git 步骤

## 1. 版本定位

V2.3 在 V2.2 `Transport / Transmitter / Dispatcher / Receiver` 抽象基础上，新增可选的真实 FastDDS/Fast-RTPS 适配层。默认构建不强依赖 FastDDS，保证普通 VMware/Ubuntu 环境仍可编译运行；安装 FastDDS/Fast-RTPS 2.12.x 后，可通过 `AVM_ENABLE_FASTDDS=ON` 启用真实 RTPS Writer/Reader 代码路径。

## 2. 从 V2.3 代码包复制、解压并合并到当前 V2.2 工程

以下命令从 `autovision_mini_middleware_v2_3_fastdds_rtps_source.zip` 复制和解压开始，直接合并到你当前已有的 V2.2 工程目录 `~/projects/autovision_mini_middleware_v1`。该 zip 包内顶层目录为 `autovision_v23/`，合并时会按相同相对路径覆盖 V2.2 中需要修改的文件，并新增 V2.3 文件。

```bash
mkdir -p ~/projects
cp /mnt/data/autovision_mini_middleware_v2_3_fastdds_rtps_source.zip ~/projects/
cd ~/projects
rm -rf autovision_v23
unzip -o autovision_mini_middleware_v2_3_fastdds_rtps_source.zip

cd ~/projects/autovision_mini_middleware_v1
git status --short
git checkout v2.2-transport-pattern
git pull origin v2.2-transport-pattern
git checkout -b v2.3-fastdds-rtps-adapter

rsync -av \
  --exclude 'build/' \
  --exclude 'logs/' \
  --exclude 'assets/' \
  --exclude 'examples/bin/' \
  --exclude 'examples/logs/' \
  ~/projects/autovision_v23/ ./

chmod +x scripts/*.sh
git status --short
```

如果你的工程目录不是 `~/projects/autovision_mini_middleware_v1`，只需要把上述 `cd ~/projects/autovision_mini_middleware_v1` 改成你的实际工程路径。合并后不要直接提交，先执行下面的普通环境验证；验证通过后再按 Git 提交建议分批提交。

## 3. 新增文件

```text
include/fastdds_rtps_transport.hpp
src/fastdds_rtps_transport.cpp
src/fastdds_rtps_loopback_demo.cpp
examples/cpp/23_fastdds_packet_codec.cpp
scripts/benchmark_fastdds_rtps.sh
docs/validation_v2_3_fastdds_rtps.md
docs/V2_3_新增命令与Git步骤.md
```

## 4. 修改文件

```text
CMakeLists.txt
README.md
examples/Makefile
include/transport_endpoint.hpp
src/transport_endpoint.cpp
```

## 5. 普通环境验证

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh
./scripts/run_all_vm.sh
./build/transport_pattern_demo --backend both --mode all --frames 5 --depth 8 --output logs/transport_pattern_v23_final.csv
./scripts/benchmark_fastdds_rtps.sh 5 8
cd examples
make bin/23_fastdds_packet_codec
make run EXAMPLE=23_fastdds_packet_codec
```

## 6. FastDDS/Fast-RTPS 2.12.x 环境验证

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
./build/fastdds_rtps_loopback_demo --frames 5 --depth 8 --topic avm/v2_3/fastdds/raw --output logs/benchmark_v2_3_fastdds/fastdds_rtps_real.csv
```

## 7. Git 提交建议

```bash
cd ~/projects/autovision_mini_middleware_v1

git add include/fastdds_rtps_transport.hpp src/fastdds_rtps_transport.cpp CMakeLists.txt
git commit -m "v2.3-step1: add optional FastDDS RTPS transport adapter"

git add include/transport_endpoint.hpp src/transport_endpoint.cpp src/fastdds_rtps_loopback_demo.cpp scripts/benchmark_fastdds_rtps.sh
git commit -m "v2.3-step2: integrate FastDDS backend into Transport endpoint pattern"

git add examples/cpp/23_fastdds_packet_codec.cpp examples/Makefile
git commit -m "v2.3-step3: add FastDDS packet codec example"

git add docs/validation_v2_3_fastdds_rtps.md docs/V2_3_新增命令与Git步骤.md README.md
git commit -m "v2.3-step4: document FastDDS RTPS validation and usage"

git push -u origin v2.3-fastdds-rtps-adapter
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

