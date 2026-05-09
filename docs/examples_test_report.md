# Examples Test Report

## Date

V1 examples module tests verified on Ubuntu 22.04 / VMware.

## Result

All C++ examples compiled successfully with the shared examples/Makefile.

Verified examples:

- 00_crc32: passed
- 01_latency_profiler: passed
- 02_control_cmd: passed
- 03_file_adapter: passed
- 04_lidar_sim_adapter: passed
- 05_shm_frame_pool: passed
- 06_shm_ring_buffer: passed
- 07_shared_status: passed
- 08_frame_ipc: passed
- 09_sensor_to_shm: passed
- 10_control_query: passed

## Notes

control_service is a foreground server and waits for Unix Domain Socket clients. This is expected behavior.

The early shm_open failed messages in 08_frame_ipc are retry logs from the consumer waiting for the producer to create shared memory. The test passed because all frames were received with crc_ok=true.
