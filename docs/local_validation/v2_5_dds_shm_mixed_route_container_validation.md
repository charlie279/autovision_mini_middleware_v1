# V2.5 DDS + SHM Mixed Route Container Validation

## Environment

- Code baseline: V2.4 taskbook extracted source + V2.5 modifications in this package.
- FastDDS availability in current container: not installed / not compiled.
- Validation scope: dependency-free build, main SHM pipeline regression, examples 23/24/25, V2.5 NOT_COMPILED fallback.

## Results

```text
./scripts/build.sh：PASS
./scripts/run_all_vm.sh：PASS
examples/23_fastdds_packet_codec：PASS
examples/24_camera_fastdds_packet：PASS
examples/25_dds_shm_frame_meta：PASS
./scripts/benchmark_camera_dds_shm.sh 5 640 480 30 yuyv ...：NOT_COMPILED fallback PASS
```

## Main pipeline

```text
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"
```

## Example output

```text
[25_dds_shm_frame_meta] metadata_bytes=208 raw_payload_bytes=614400 size_errors=0 payload_errors=0 result=PASS
```

## Fallback output

```text
pub,false,synthetic,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,5,10,1,64,614400,0,0,0,0,0,0,0,NOT_COMPILED
sub,false,avm/camera/dds_shm_synthetic_meta,/avm_camera_dds_shm_synthetic_pool,yuyv,640,480,5,11,0,0,0,0,0,0,0,120000,0,0,NOT_COMPILED
```

## Waiting for user-side FastDDS verification

```text
1. Rebuild with -DAVM_ENABLE_FASTDDS=ON.
2. Run synthetic 300-frame DDS+SHM validation.
3. Run real USB camera 300-frame DDS+SHM validation.
4. Compare V2.4 raw DDS vs V2.5 metadata DDS + SHM payload latency and CPU metrics.
```
