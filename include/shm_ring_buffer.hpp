/**
 * @file shm_ring_buffer.hpp
 * @brief POSIX 共享内存环形队列：只传 FrameMeta/TensorMeta，不传图像大数据。
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <pthread.h>
#include <string>

struct RingHeader {
    std::uint32_t magic = 0;
    std::uint32_t elem_size = 0;
    std::uint32_t capacity = 0;
    std::uint32_t initialized = 0;
    std::uint64_t head = 0;
    std::uint64_t tail = 0;
    std::uint64_t count = 0;
    std::uint64_t drop_count = 0;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

class ShmRingBuffer {
public:
    ShmRingBuffer() = default;
    ~ShmRingBuffer();

    bool create(const std::string& name, std::size_t elem_size, std::size_t capacity);
    bool open(const std::string& name, std::size_t elem_size, std::size_t capacity);
    bool push(const void* item, std::size_t elem_size);
    bool try_push_drop_oldest(const void* item, std::size_t elem_size);
    std::uint64_t depth() const;
    std::uint64_t capacity() const;
    std::uint64_t drop_count() const;
    bool pop(void* item, std::size_t elem_size, int timeout_ms = -1);
    void close();

private:
    bool map_common(const std::string& name, std::size_t elem_size,
                    std::size_t capacity, bool create_mode);
    std::uint8_t* data_base();
    const std::uint8_t* data_base() const;

    int fd_ = -1;
    void* base_ = nullptr;
    RingHeader* header_ = nullptr;
    std::size_t total_size_ = 0;
    std::size_t elem_size_ = 0;
    std::size_t capacity_ = 0;
};
