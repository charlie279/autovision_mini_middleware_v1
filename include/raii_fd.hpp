
/**
 * @file raii_fd.hpp
 * @brief Minimal RAII wrappers for Linux file descriptors and mmap regions.
 */
#pragma once

#include <cstddef>
#include <utility>
#include <sys/mman.h>
#include <unistd.h>

namespace avm {

class UniqueFd {
public:
    UniqueFd() = default;
    explicit UniqueFd(int fd) : fd_(fd) {}
    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    explicit operator bool() const { return fd_ >= 0; }

    int release() {
        const int fd = fd_;
        fd_ = -1;
        return fd;
    }

    void reset(int fd = -1) {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = fd;
    }

private:
    int fd_ = -1;
};

class MmapRegion {
public:
    MmapRegion() = default;
    MmapRegion(void* addr, std::size_t length) : addr_(addr), length_(length) {}
    ~MmapRegion() { reset(); }

    MmapRegion(const MmapRegion&) = delete;
    MmapRegion& operator=(const MmapRegion&) = delete;

    MmapRegion(MmapRegion&& other) noexcept : addr_(other.addr_), length_(other.length_) {
        other.addr_ = nullptr;
        other.length_ = 0;
    }
    MmapRegion& operator=(MmapRegion&& other) noexcept {
        if (this != &other) {
            reset();
            addr_ = other.addr_;
            length_ = other.length_;
            other.addr_ = nullptr;
            other.length_ = 0;
        }
        return *this;
    }

    void* get() const { return addr_; }
    std::size_t size() const { return length_; }
    explicit operator bool() const { return addr_ != nullptr && addr_ != MAP_FAILED && length_ > 0; }

    void reset(void* addr = nullptr, std::size_t length = 0) {
        if (addr_ != nullptr && addr_ != MAP_FAILED && length_ > 0) {
            ::munmap(addr_, length_);
        }
        addr_ = addr;
        length_ = length;
    }

private:
    void* addr_ = nullptr;
    std::size_t length_ = 0;
};

}  // namespace avm
