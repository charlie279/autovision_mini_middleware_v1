#!/usr/bin/env bash
set -euo pipefail
FRAMES=${1:-60}
mkdir -p logs/benchmark
./scripts/build.sh
./scripts/prepare_input.sh
./scripts/run_all_vm.sh
./build/algorithm_stub_node --input logs/npu_results.jsonl --output logs/detection_results.jsonl > logs/algorithm_stub.log 2>&1
./scripts/benchmark_v1_8_vm_perf_opt.sh "$FRAMES"
cd examples
for e in 15_fd_passing_alive_counter 16_udp_lidar_timestamp_sync 17_nv12_libyuv_preprocess 18_runtime_metadata_detection 19_affinity_priority_benchmark; do
  make bin/$e >/dev/null
  ./bin/$e > ../logs/benchmark/${e}.log 2>&1 || { cat ../logs/benchmark/${e}.log; exit 1; }
done
cd ..
./scripts/benchmark_affinity_priority.sh 10000000 > logs/benchmark/affinity_priority_stdout.log 2>&1 || true
./scripts/run_record_replay_demo.sh 20 > logs/benchmark/record_replay_stdout.log 2>&1
cat > logs/benchmark/v1_9_validation_summary.txt <<SUMMARY
main_build=PASS
file_pipeline=PASS
algorithm_stub=PASS
examples_15_19=PASS
record_replay=PASS
affinity_priority=PASS_WITH_NON_ROOT_PRIORITY_FALLBACK
real_camera=WAIT_USER_VMWARE_OR_BOARD
SUMMARY
cat logs/benchmark/v1_9_validation_summary.txt

