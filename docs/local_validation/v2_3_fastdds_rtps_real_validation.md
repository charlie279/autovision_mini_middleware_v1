# V2.3 Real FastDDS/FastRTPS Validation Report

## Environment

Project branch: v2.3-fastdds-rtps-adapter
Base branch: v2.2-transport-pattern
OS: VMware Ubuntu
Compiler: GNU 11.4.0
FastDDS/FastRTPS install prefix: /home/charlie/Fast-DDS/install
FastRTPS library: /home/charlie/Fast-DDS/install/lib/libfastrtps.so.2.12.0
FastCDR library: /home/charlie/Fast-DDS/install/lib/libfastcdr.so.2.0.0
Build option: -DAVM_ENABLE_FASTDDS=ON

## Dependency Discovery

CMake config files found:

- /home/charlie/Fast-DDS/install/lib/cmake/fastcdr/fastcdr-config.cmake
- /home/charlie/Fast-DDS/install/share/fastrtps/cmake/fastrtps-config.cmake

Runtime libraries found:

- /home/charlie/Fast-DDS/install/lib/libfastcdr.so.2.0.0
- /home/charlie/Fast-DDS/install/lib/libfastrtps.so.2.12.0

## Build With FastDDS Enabled

Command:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH="$FASTDDS_PREFIX"
cmake --build build -j"$(nproc)"

Observed result:

PASS

Built targets include:

- fastdds_rtps_loopback_demo
- transport_four_links_demo
- transport_pattern_demo

## Real FastDDS/FastRTPS Loopback Demo

Command:

./build/fastdds_rtps_loopback_demo --frames 5 --depth 8 --topic avm/v2_3/fastdds/raw --output logs/benchmark_v2_3_fastdds_real/fastdds_rtps_real.csv

Observed result:

[fastdds_rtps_loopback_demo] compiled=true sent=5 received=5 size_errors=0 payload_errors=0 result=PASS

CSV result:

backend,compiled,topic,frames,sent,received,size_errors,payload_errors,status
fastdds,true,avm/v2_3/fastdds/raw,5,5,5,0,0,PASS

## Script-Level FastDDS Benchmark

Command:

./scripts/benchmark_fastdds_rtps.sh 5 8

Observed result:

[fastdds_rtps_loopback_demo] compiled=true sent=5 received=5 size_errors=0 payload_errors=0 result=PASS

CSV result:

backend,compiled,topic,frames,sent,received,size_errors,payload_errors,status
fastdds,true,avm/v2_3/fastdds/raw,5,5,5,0,0,PASS

## Acceptance Status

[PASS] FastDDS/FastRTPS 2.12.0 installation detected.
[PASS] FastCDR installation detected.
[PASS] Project builds with -DAVM_ENABLE_FASTDDS=ON.
[PASS] fastdds_rtps_loopback_demo runs with real FastRTPS library.
[PASS] sent=5, received=5.
[PASS] size_errors=0.
[PASS] payload_errors=0.
[PASS] status=PASS.
[PASS] benchmark_fastdds_rtps.sh no longer reports NOT_COMPILED.

## Remaining Boundary

This validation proves local FastDDS/FastRTPS adapter compilation and local loopback transport validation.

Pending for later versions:

1. Cross-process FastDDS publisher/subscriber split.
2. Multi-topic long-running soak test.
3. QoS overflow and drop-policy stress test.
4. Integration with real camera/raw frame pipeline as DDS message source.
5. OPi5+/RK3588 board-side validation.
