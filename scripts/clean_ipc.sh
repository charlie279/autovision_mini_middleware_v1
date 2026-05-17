#!/usr/bin/env bash
set -euo pipefail
for proc in media_node preprocess_node npu_stub_node safety_monitor control_service; do
    pkill "$proc" 2>/dev/null || true
done
rm -f /dev/shm/avm_frame_pool \
      /dev/shm/avm_frame_meta_ring \
      /dev/shm/avm_tensor_pool \
      /dev/shm/avm_tensor_meta_ring \
      /dev/shm/avm_runtime_status
rm -f /tmp/avm_control.sock
echo "[clean_ipc] done"
