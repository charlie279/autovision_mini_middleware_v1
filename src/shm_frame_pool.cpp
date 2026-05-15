/**
 * @file shm_frame_pool.cpp
 * @brief POSIX SHM FramePool 实现：创建/打开共享内存并按 buffer_index 读写 payload。
 */
#include "shm_frame_pool.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr std::uint32_t kPoolMagic = 0x41564D50U;  // 'AVMP'
}

ShmFramePool::~ShmFramePool() {
    close();
}

bool ShmFramePool::create(const std::string& name, std::size_t buffer_count, std::size_t buffer_size) {
    return map_common(name, buffer_count, buffer_size, true);
}

bool ShmFramePool::open(const std::string& name, std::size_t buffer_count, std::size_t buffer_size) {
    return map_common(name, buffer_count, buffer_size, false);
}

bool ShmFramePool::map_common(const std::string& name,
                              std::size_t buffer_count,
                              std::size_t buffer_size,
                              bool create_mode) {
    close();

    buffer_count_ = buffer_count;
    buffer_size_ = buffer_size;
    total_size_ = sizeof(FramePoolHeader) + buffer_count_ * buffer_size_;

    const int flags = O_RDWR | (create_mode ? O_CREAT : 0);
    fd_ = shm_open(name.c_str(), flags, 0666);
    if (fd_ < 0) {
        std::cerr << "[ShmFramePool] shm_open failed: " << name << " " << std::strerror(errno) << "\n";
        return false;
    }

    if (create_mode && ftruncate(fd_, static_cast<off_t>(total_size_)) != 0) {
        std::cerr << "[ShmFramePool] ftruncate failed: " << std::strerror(errno) << "\n";
        close();
        return false;
    }

    base_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (base_ == MAP_FAILED) {
        std::cerr << "[ShmFramePool] mmap failed: " << std::strerror(errno) << "\n";
        base_ = nullptr;
        close();
        return false;
    }

    auto* header = static_cast<FramePoolHeader*>(base_);
    if (create_mode || header->magic != kPoolMagic) {
        header->magic = kPoolMagic;
        header->buffer_count = static_cast<std::uint32_t>(buffer_count_);
        header->buffer_size = static_cast<std::uint32_t>(buffer_size_);
        header->reserved = 0;
    }

    return true;
}

bool ShmFramePool::write(std::uint32_t index, const void* data, std::size_t size) {
    if (base_ == nullptr || data == nullptr || index >= buffer_count_ || size > buffer_size_) {
        return false;
    }
    std::memcpy(buffer_ptr(index), data, size);
    return true;
}

bool ShmFramePool::read(std::uint32_t index, void* data, std::size_t size) const {
    if (base_ == nullptr || data == nullptr || index >= buffer_count_ || size > buffer_size_) {
        return false;
    }
    std::memcpy(data, buffer_ptr(index), size);
    return true;
}

std::uint8_t* ShmFramePool::buffer_ptr(std::uint32_t index) {
    if (base_ == nullptr || index >= buffer_count_) {
        return nullptr;
    }
    return static_cast<std::uint8_t*>(base_) + sizeof(FramePoolHeader) +
           static_cast<std::size_t>(index) * buffer_size_;
}

const std::uint8_t* ShmFramePool::buffer_ptr(std::uint32_t index) const {
    if (base_ == nullptr || index >= buffer_count_) {
        return nullptr;
    }
    return static_cast<const std::uint8_t*>(base_) + sizeof(FramePoolHeader) +
           static_cast<std::size_t>(index) * buffer_size_;
}

std::size_t ShmFramePool::buffer_count() const {
    return buffer_count_;
}

std::size_t ShmFramePool::buffer_size() const {
    return buffer_size_;
}

void ShmFramePool::close() {
    if (base_ != nullptr) {
        munmap(base_, total_size_);
        base_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}