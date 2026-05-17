# V2.0 新增代码放置、运行命令与 Git 步骤

## 1. 解压覆盖

将 zip 包放到工程父目录，例如：

```bash
mkdir -p ~/projects
cd ~/projects
unzip -o autovision_mini_middleware_v2_0_video_encode_source.zip
cd autovision_mini_middleware_v1
chmod +x scripts/*.sh
```

如果你已经有远端 V1.9 分支代码，也可以只把 zip 中的新增/修改文件复制到原工程根目录。

## 2. 依赖

基础依赖：

```bash
sudo apt update
sudo apt install -y build-essential cmake git python3 ffmpeg
```

可选 FFmpeg 开发包，供后续 libavcodec 版本实现使用：

```bash
sudo apt install -y libavcodec-dev libavutil-dev libswscale-dev
```

## 3. 编译

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh
```

可选 CMake 参数：

```bash
./scripts/build.sh -DAVM_HAS_V4L2M2M=ON
./scripts/build.sh -DAVM_HAS_MPP=ON
```

## 4. V1.9 回归验证

```bash
./scripts/run_all_vm.sh
cat logs/final_status.txt
```

预期：

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## 5. V2.0 编码链路验证

H.264：

```bash
./scripts/run_encode_pipeline.sh soft h264 30
ffprobe -v error -show_entries stream=codec_name,width,height -of default=noprint_wrappers=1 logs/encode_output.h264
```

H.265：

```bash
./scripts/run_encode_pipeline.sh soft h265 10
ffprobe -v error -show_entries stream=codec_name,width,height -of default=noprint_wrappers=1 logs/encode_output.h265
```

编码 benchmark：

```bash
./scripts/benchmark_encode.sh 30 640 480
cat logs/benchmark_v2_0/encode_benchmark.csv
```

example 20：

```bash
cd examples
make all
make run EXAMPLE=20_video_encode_benchmark
```

## 6. Git 分阶段提交

```bash
cd ~/projects/autovision_mini_middleware_v1

git checkout v1.9-middleware-featurepack
git pull
git checkout -b v2.0-video-encode

git add include/video_encoder.hpp include/video_encoder_factory.hpp \
        include/soft_video_encoder.hpp include/v4l2_m2m_encoder.hpp \
        include/mpp_video_encoder.hpp include/sensor_frame.hpp
git commit -m "v2.0-step1: add video encoder interface and backend headers"

git add src/soft_video_encoder.cpp src/v4l2_m2m_encoder.cpp \
        src/mpp_video_encoder.cpp src/video_encoder_factory.cpp
git commit -m "v2.0-step2: implement soft ffmpeg cli encoder and hardware backend stubs"

git add src/encode_sink_node.cpp CMakeLists.txt scripts/build.sh
git commit -m "v2.0-step3: add encode sink node and cmake targets"

git add examples/cpp/20_video_encode_benchmark.cpp examples/Makefile
git commit -m "v2.0-step4: add video encode benchmark example"

git add scripts/run_encode_pipeline.sh scripts/benchmark_encode.sh
git commit -m "v2.0-step5: add encode pipeline and benchmark scripts"

git add docs/validation_v2_0_video_encode.md docs/V2_0_新增命令与Git步骤.md README.md
git commit -m "v2.0-step6: add video encode validation docs"

git push -u origin v2.0-video-encode
```

