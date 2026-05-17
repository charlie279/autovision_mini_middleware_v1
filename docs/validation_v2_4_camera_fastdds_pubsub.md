# AutoVision Mini Middleware V2.4 Camera FastDDS Pub/Sub Validation

## 1. Scope

V2.4 adds two independent processes:

```text
camera_fastdds_pub: CameraAdapterV4L2 or synthetic raw frame -> FastDDS/RTPS
camera_fastdds_sub: FastDDS/RTPS -> raw frame validation / optional dump
```

This stable revision fixes the first V2.4 validation problem observed in VMware: the publisher could send all frames, while the subscriber reported `PARTIAL` because early DDS discovery/history delivery and RTPS/UDP fragmentation caused missing sequence numbers.

## 2. Stabilization changes

```text
1. camera_fastdds_pub adds --startup-ms, --linger-ms, --warmup-frames, --repeat and --inter-repeat-us.
2. camera_fastdds_sub adds --start-seq and subscriber-side de-duplication.
3. Synthetic and camera scripts now default to reliable mode, depth=512, warmup=10, start-seq=11, repeat=5.
4. PARTIAL now returns non-zero, so scripts no longer silently pass incomplete subscriber validation.
5. CSV output now includes warmup/repeat/tx_attempts/start_seq/unique_received/duplicates/lost fields.
```

## 3. Verified in this Linux container

This container does not have FastDDS/Fast-RTPS installed, so real RTPS Writer/Reader cannot be executed here. The following have been verified:

```bash
./scripts/build.sh
./scripts/run_all_vm.sh
cd examples && make bin/23_fastdds_packet_codec bin/24_camera_fastdds_packet
cd examples && make run EXAMPLE=23_fastdds_packet_codec
cd examples && make run EXAMPLE=24_camera_fastdds_packet
./scripts/benchmark_camera_fastdds_pub_sub.sh 5 320 240 30 yuyv avm/camera/test_not_compiled
```

Observed results:

```text
build: PASS
file pipeline: status=NORMAL fps=30 media/preprocess/npu=120/120/120
23_fastdds_packet_codec: PASS, NOT_COMPILED status is expected in dependency-free build
24_camera_fastdds_packet: PASS, 640x480 YUYV and RGB888 payloads pass V2.4 2MiB QoS boundary
benchmark_camera_fastdds_pub_sub.sh: runs and reports NOT_COMPILED cleanly in dependency-free build
```

## 4. User-side FastDDS validation commands

Install FastDDS/Fast-RTPS and rebuild:

```bash
cd ~/projects/autovision_mini_middleware_v1
./scripts/build.sh -DAVM_ENABLE_FASTDDS=ON -DCMAKE_PREFIX_PATH=$HOME/Fast-DDS/install
```

Synthetic 640x480 YUYV validation:

```bash
./scripts/benchmark_camera_fastdds_pub_sub.sh 60 640 480 30 yuyv avm/camera/synthetic_raw
cat logs/benchmark_v2_4_fastdds_camera_synthetic/camera_fastdds_pub.csv
cat logs/benchmark_v2_4_fastdds_camera_synthetic/camera_fastdds_sub.csv
```

Expected if the local FastDDS environment can sustain the stream:

```text
pub,true,synthetic,...,frames_requested=60,warmup_frames=10,repeat=5,...,sent=60,...,PASS
sub,true,...,frames_expected=60,start_seq=11,unique_received=60,...,lost=0,...,PASS
```

USB camera validation:

```bash
./scripts/run_camera_fastdds_pub_sub.sh /dev/video0 60 640 480 30 yuyv avm/camera/raw
cat logs/benchmark_v2_4_fastdds_camera/camera_fastdds_pub.csv
cat logs/benchmark_v2_4_fastdds_camera/camera_fastdds_sub.csv
```

## 5. Boundary

If the subscriber is still `PARTIAL` after this revision, the remaining problem is not AutoVision packet codec or payload-size boundary. It should be treated as a true FastDDS/RTPS resource/QoS/network-fragmentation tuning issue in the user VM, and the next version should move to DDS+SHM hybrid routing or a higher-level typed FastDDS DataWriter/DataReader implementation.
