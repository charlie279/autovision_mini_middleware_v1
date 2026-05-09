# V1 Runtime Report

## Final Status
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"

## Media Log Tail
[media_node] frame_id=1 sensor_type=2 size=921600 buffer_index=1
[media_node] frame_id=30 sensor_type=2 size=921600 buffer_index=6
[media_node] frame_id=60 sensor_type=2 size=921600 buffer_index=4
[media_node] frame_id=90 sensor_type=2 size=921600 buffer_index=2
[media_node] frame_id=120 sensor_type=2 size=921600 buffer_index=0
[media_node] finished produced=120

## Preprocess Log Tail
[ShmFramePool] shm_open failed: /avm_frame_pool No such file or directory
[ShmFramePool] shm_open failed: /avm_frame_pool No such file or directory
[ShmFramePool] shm_open failed: /avm_frame_pool No such file or directory
[Preprocess] frame_id=1 resize+normalize=1.58665ms total=7.76652ms mean=7.76652ms p95=7.76652ms
[Preprocess] frame_id=30 resize+normalize=0.831868ms total=6.29426ms mean=5.10908ms p95=6.75572ms
[Preprocess] frame_id=60 resize+normalize=0.54213ms total=4.64281ms mean=4.98902ms p95=6.31632ms
[Preprocess] frame_id=90 resize+normalize=0.561646ms total=4.76206ms mean=4.88899ms p95=6.23899ms
[Preprocess] frame_id=120 resize+normalize=0.544884ms total=4.61402ms mean=4.85796ms p95=6.23037ms
[preprocess_node] finished processed=120 crc_errors=0 frame_jumps=0

## NPU Log Tail
[ShmRingBuffer] shm_open failed: /avm_tensor_meta_ring No such file or directory
[ShmRingBuffer] shm_open failed: /avm_tensor_meta_ring No such file or directory
[ShmRingBuffer] shm_open failed: /avm_tensor_meta_ring No such file or directory
[ShmRingBuffer] shm_open failed: /avm_tensor_meta_ring No such file or directory
[NpuEngine] init model=models/fake_model.tflite (stub)
[NPUStub] {"frame_id":1,"object_count":4,"confidence":0.81975,"npu_latency_ms":8.12344}
[NPUStub] {"frame_id":30,"object_count":1,"confidence":0.666562,"npu_latency_ms":8.15746}
[NPUStub] {"frame_id":60,"object_count":2,"confidence":0.635026,"npu_latency_ms":8.11496}
[NPUStub] {"frame_id":90,"object_count":3,"confidence":0.972721,"npu_latency_ms":8.09878}
[NPUStub] {"frame_id":120,"object_count":1,"confidence":0.7835,"npu_latency_ms":8.1316}
[NpuEngine] release
[npu_stub_node] finished consumed=120

## Safety Log Tail
[Safety] state=NORMAL media=31 preprocess=31 npu=30 crc_errors=0 frame_jumps=0
[Safety] state=NORMAL media=61 preprocess=61 npu=61 crc_errors=0 frame_jumps=0
[Safety] state=NORMAL media=91 preprocess=91 npu=91 crc_errors=0 frame_jumps=0
[Safety] state=NORMAL media=120 preprocess=120 npu=120 crc_errors=0 frame_jumps=0
