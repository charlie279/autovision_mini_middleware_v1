/**
 * @file 08_frame_consumer.cpp
 * @brief 双进程 IPC 示例消费者：从 Ring 读取 FrameMeta，再按 buffer_index 从 FramePool 读取 payload。
 */
#include "crc32.hpp"
#include "frame_meta.hpp"
#include "shm_frame_pool.hpp"
#include "shm_ring_buffer.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
template <typename OpenFunc>
bool retry_open(OpenFunc&& open_func, const char* name, int retry_count = 50) {
    for (int i = 0; i < retry_count; ++i) {
        if (open_func()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cerr << "[08_frame_consumer] failed to open " << name << "\n";
    return false;
}
}  // namespace

int main() {
    constexpr const char* kPoolName = "/avm_example_frame_pool";
    constexpr const char* kRingName = "/avm_example_frame_meta_ring";
    constexpr std::size_t kBufferCount = 4;
    constexpr std::size_t kBufferSize = 4096;
    constexpr std::size_t kRingCapacity = 8;

    ShmFramePool pool;
    ShmRingBuffer ring;
    if (!retry_open([&]() { return pool.open(kPoolName, kBufferCount, kBufferSize); }, "frame_pool")) {
        return 1;
    }
    if (!retry_open([&]() { return ring.open(kRingName, sizeof(FrameMeta), kRingCapacity); }, "ring")) {
        return 2;
    }

    for (int i = 0; i < 5; ++i) {
        FrameMeta meta;
        if (!ring.pop(&meta, sizeof(meta), 3000)) {
            std::cerr << "[08_frame_consumer] pop timeout\n";
            return 3;
        }

        std::vector<char> payload(meta.data_size);
        pool.read(meta.buffer_index, payload.data(), payload.size());
        const std::uint32_t actual_crc = crc32_compute(payload.data(), payload.size());

        std::cout << "[08_frame_consumer] frame_id=" << meta.frame_id
                  << " payload=\"" << payload.data() << "\""
                  << " crc_ok=" << (actual_crc == meta.crc32 ? "true" : "false") << "\n";
    }
    return 0;
}