/**
 * @file shm_frame_pool.hpp
 * @brief POSIX 共享内存帧池：存放图像/Tensor payload，跨进程通过 buffer_index 访问。
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct FramePoolHeader {
    std::uint32_t magic = 0;
    std::uint32_t buffer_count = 0;
    std::uint32_t buffer_size = 0;
    std::uint32_t reserved = 0;
};

class ShmFramePool {
public:
    ShmFramePool() = default;
    ~ShmFramePool();

    bool create(const std::string& name, std::size_t buffer_count, std::size_t buffer_size);
    bool open(const std::string& name, std::size_t buffer_count, std::size_t buffer_size);
    bool write(std::uint32_t index, const void* data, std::size_t size);
    bool read(std::uint32_t index, void* data, std::size_t size) const;

    std::uint8_t* buffer_ptr(std::uint32_t index);
    const std::uint8_t* buffer_ptr(std::uint32_t index) const;

    std::size_t buffer_count() const;
    std::size_t buffer_size() const;
    void close();

private:
    bool map_common(const std::string& name, std::size_t buffer_count,
                    std::size_t buffer_size, bool create_mode);

    int fd_ = -1;
    void* base_ = nullptr;
    std::size_t total_size_ = 0;
    std::size_t buffer_count_ = 0;
    std::size_t buffer_size_ = 0;
};
