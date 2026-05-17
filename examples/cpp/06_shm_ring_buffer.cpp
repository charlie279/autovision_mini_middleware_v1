/**
 * @file 06_shm_ring_buffer.cpp
 * @brief 示例：调用 ShmRingBuffer::create/push/pop，理解 Ring 中只传 FrameMeta。
 */
#include "frame_meta.hpp"
#include "shm_ring_buffer.hpp"

#include <iostream>
#include <string>
#include <sys/mman.h>

int main() {
    const std::string name = "/avm_example_frame_meta_ring";
    shm_unlink(name.c_str());

    ShmRingBuffer ring;
    if (!ring.create(name, sizeof(FrameMeta), 8)) {
        std::cerr << "[06_shm_ring_buffer] create failed\n";
        return 1;
    }

    FrameMeta input;
    input.frame_id = 42;
    input.width = 640;
    input.height = 480;
    input.buffer_index = 2;
    input.crc32 = 0x12345678U;

    if (!ring.push(&input, sizeof(input))) {
        std::cerr << "[06_shm_ring_buffer] push failed\n";
        return 2;
    }

    FrameMeta output;
    if (!ring.pop(&output, sizeof(output), 1000)) {
        std::cerr << "[06_shm_ring_buffer] pop failed\n";
        return 3;
    }

    std::cout << "[06_shm_ring_buffer] frame_id=" << output.frame_id
              << " width=" << output.width
              << " height=" << output.height
              << " buffer_index=" << output.buffer_index
              << " crc32=0x" << std::hex << output.crc32 << std::dec << "\n";

    ring.close();
    shm_unlink(name.c_str());
    return 0;
}
