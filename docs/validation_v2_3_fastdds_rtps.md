# AutoVision Mini Middleware V2.3 FastDDS/RTPS Validation

## 1. Scope

V2.3 adds an optional real FastDDS/Fast-RTPS adapter layer for AutoVision `TransportEnvelope` messages. The adapter is implemented in AutoVision-owned files and does not copy third-party source headers, author blocks, project namespaces, or project-specific macros from the uploaded reference archive.

## 2. Files added or modified

```text
include/fastdds_rtps_transport.hpp
src/fastdds_rtps_transport.cpp
src/fastdds_rtps_loopback_demo.cpp
examples/cpp/23_fastdds_packet_codec.cpp
scripts/benchmark_fastdds_rtps.sh
CMakeLists.txt
README.md
examples/Makefile
include/transport_endpoint.hpp
src/transport_endpoint.cpp
```

## 3. Verified in this Linux container

```bash
./scripts/build.sh
./scripts/run_all_vm.sh
./build/transport_four_links_demo --mode all --frames 5 --output logs/transport_four_links_v23_final.csv
./build/transport_pattern_demo --backend both --mode all --frames 5 --depth 8 --output logs/transport_pattern_v23_final.csv
./scripts/run_encode_pipeline.sh soft h264 5
./scripts/benchmark_fastdds_rtps.sh 5 8
cd examples && make bin/23_fastdds_packet_codec && make run EXAMPLE=23_fastdds_packet_codec
```

Observed results:

```text
build: PASS
file pipeline: status=NORMAL fps=30 media/preprocess/npu=120/120/120
transport_four_links_demo: all scenarios PASS
transport_pattern_demo: rtps/shm local backends all scenarios PASS
H.264 soft encode: ffprobe codec_name=h264,width=640,height=480
fastdds_rtps_loopback_demo: NOT_COMPILED because FastDDS/Fast-RTPS is not installed in this container
23_fastdds_packet_codec: PASS, encoded_bytes=160096, lidar payload=160000, decode=OK, validate=OK
```

## 4. Waiting for user-side verification

```text
1. Install FastDDS/Fast-RTPS 2.12.x and Fast-CDR.
2. Rebuild with -DAVM_ENABLE_FASTDDS=ON and the correct CMAKE_PREFIX_PATH.
3. Run fastdds_rtps_loopback_demo without --allow-unavailable.
4. Run two-process publisher/subscriber verification if you split the demo into separate processes.
5. Test cross-host UDP discovery only after confirming local loopback works.
```

## 5. Commands for real FastDDS/Fast-RTPS 2.12.x environment

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
./build/fastdds_rtps_loopback_demo --frames 5 --depth 8 --topic avm/v2_3/fastdds/raw --output logs/benchmark_v2_3_fastdds/fastdds_rtps_real.csv
cat logs/benchmark_v2_3_fastdds/fastdds_rtps_real.csv
```

Expected result after the dependency is correctly installed:

```text
backend,compiled,topic,frames,sent,received,size_errors,payload_errors,status
fastdds,true,avm/v2_3/fastdds/raw,5,5,5,0,0,PASS
```

