# V2.3 FastDDS/RTPS Container Validation Snapshot

```text
./scripts/build.sh: PASS
./scripts/run_all_vm.sh: PASS, status=NORMAL fps=30 media/preprocess/npu=120/120/120
./build/transport_four_links_demo --mode all --frames 5: PASS for all scenarios
./build/transport_pattern_demo --backend both --mode all --frames 5 --depth 8: PASS for rtps/shm local backends
./scripts/run_encode_pipeline.sh soft h264 5 + ffprobe: codec_name=h264,width=640,height=480
./scripts/benchmark_fastdds_rtps.sh 5 8: runnable; reports NOT_COMPILED because FastDDS/Fast-RTPS is not installed in this container
examples/cpp/23_fastdds_packet_codec.cpp: PASS, encoded_bytes=160096, payload=160000, decode=OK, validate=OK
```
