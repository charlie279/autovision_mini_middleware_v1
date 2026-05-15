/**
 * @file 08_frame_producer.cpp
 * @brief 双进程 IPC 示例生产者：写 payload 到 FramePool，再推送 FrameMeta 到 Ring。
 */
#include "crc32.hpp"
#include "frame_meta.hpp"
#include "shm_frame_pool.hpp"
#include "shm_ring_buffer.hpp"
#include "time_utils.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

int main() {
    constexpr const char* kPoolName = "/avm_example_frame_pool";
    constexpr const char* kRingName = "/avm_example_frame_meta_ring";
    constexpr std::size_t kBufferCount = 4;
    constexpr std::size_t kBufferSize = 4096;
    constexpr std::size_t kRingCapacity = 8;

    ShmFramePool pool;
    ShmRingBuffer ring;
    if (!pool.create(kPoolName, kBufferCount, kBufferSize)) {
        std::cerr << "[08_frame_producer] pool create failed\n";
        return 1;
    }
    if (!ring.create(kRingName, sizeof(FrameMeta), kRingCapacity)) {
        std::cerr << "[08_frame_producer] ring create failed\n";
        return 2;
    }

    for (std::uint64_t frame_id = 1; frame_id <= 5; ++frame_id) {
        std::string text = "payload for frame " + std::to_string(frame_id);
        const std::uint32_t index = static_cast<std::uint32_t>(frame_id % kBufferCount);
        pool.write(index, text.data(), text.size() + 1);

        FrameMeta meta;
        meta.frame_id = frame_id;
        meta.timestamp_ns = avm::now_ns();
        meta.width = 1;
        meta.height = 1;
        meta.data_size = static_cast<std::uint32_t>(text.size() + 1);
        meta.buffer_index = index;
        meta.crc32 = crc32_compute(text.data(), text.size() + 1);
        meta.alive_counter = static_cast<std::uint32_t>(frame_id);

        ring.push(&meta, sizeof(meta));
        std::cout << "[08_frame_producer] pushed frame_id=" << frame_id
                  << " buffer_index=" << index << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return 0;
}