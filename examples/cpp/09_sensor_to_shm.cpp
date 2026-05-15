/**
 * @file 09_sensor_to_shm.cpp
 * @brief 示例：把 FileAdapter 读出的 SensorFrame 写入 FramePool，并通过 Ring 传递 FrameMeta。
 */
#include "avm_config.hpp"
#include "crc32.hpp"
#include "file_adapter.hpp"
#include "frame_meta.hpp"
#include "shm_frame_pool.hpp"
#include "shm_ring_buffer.hpp"

#include <iostream>
#include <string>
#include <sys/mman.h>
#include <vector>

int main() {
    const std::string pool_name = "/avm_example_sensor_pool";
    const std::string ring_name = "/avm_example_sensor_ring";
    shm_unlink(pool_name.c_str());
    shm_unlink(ring_name.c_str());

    FileAdapter adapter("assets/input_640x480_rgb.raw",
                        avm::kDefaultWidth,
                        avm::kDefaultHeight,
                        avm::kDefaultChannels);
    if (!adapter.open() || !adapter.start()) {
        std::cerr << "[09_sensor_to_shm] FileAdapter failed. Run: cd .. && ./scripts/prepare_input.sh\n";
        return 1;
    }

    ShmFramePool pool;
    ShmRingBuffer ring;
    if (!pool.create(pool_name, avm::kFramePoolCount, avm::kFrameBufferSize)) {
        return 2;
    }
    if (!ring.create(ring_name, sizeof(FrameMeta), avm::kRingCapacity)) {
        return 3;
    }

    SensorFrame frame;
    if (!adapter.readFrame(frame)) {
        std::cerr << "[09_sensor_to_shm] readFrame failed\n";
        return 4;
    }

    FrameMeta meta;
    meta.frame_id = frame.frame_id;
    meta.timestamp_ns = frame.timestamp_ns;
    meta.sensor_type = frame.sensor_type;
    meta.width = frame.width;
    meta.height = frame.height;
    meta.format = frame.format;
    meta.data_size = frame.data_size;
    meta.buffer_index = static_cast<std::uint32_t>(meta.frame_id % avm::kFramePoolCount);
    meta.crc32 = crc32_compute(frame.data.data(), frame.data.size());
    meta.alive_counter = 1;

    pool.write(meta.buffer_index, frame.data.data(), frame.data.size());
    ring.push(&meta, sizeof(meta));

    FrameMeta received;
    ring.pop(&received, sizeof(received), 1000);
    std::vector<std::uint8_t> payload(received.data_size);
    pool.read(received.buffer_index, payload.data(), payload.size());
    const bool crc_ok = crc32_compute(payload.data(), payload.size()) == received.crc32;

    std::cout << "[09_sensor_to_shm] frame_id=" << received.frame_id
              << " data_size=" << received.data_size
              << " buffer_index=" << received.buffer_index
              << " crc_ok=" << (crc_ok ? "true" : "false") << "\n";

    adapter.stop();
    shm_unlink(pool_name.c_str());
    shm_unlink(ring_name.c_str());
    return crc_ok ? 0 : 5;
}