/**
 * @file 05_shm_frame_pool.cpp
 * @brief 示例：调用 ShmFramePool::create/write/read，理解 payload 共享内存池访问方式。
 */
#include "shm_frame_pool.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/mman.h>

int main() {
    const std::string name = "/avm_example_frame_pool";
    shm_unlink(name.c_str());

    ShmFramePool pool;
    if (!pool.create(name, 4, 1024)) {
        std::cerr << "[05_shm_frame_pool] create failed\n";
        return 1;
    }

    const std::string input = "frame payload example";
    if (!pool.write(0, input.data(), input.size() + 1)) {
        std::cerr << "[05_shm_frame_pool] write failed\n";
        return 2;
    }

    char output[1024]{};
    if (!pool.read(0, output, input.size() + 1)) {
        std::cerr << "[05_shm_frame_pool] read failed\n";
        return 3;
    }

    std::cout << "[05_shm_frame_pool] buffer_count=" << pool.buffer_count()
              << " buffer_size=" << pool.buffer_size()
              << " output=\"" << output << "\"\n";

    pool.close();
    shm_unlink(name.c_str());
    return 0;
}
