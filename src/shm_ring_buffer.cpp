/**
 * @file shm_ring_buffer.cpp
 * @brief POSIX SHM RingBuffer 实现：跨进程阻塞式 push/pop 元数据。
 */
#include "shm_ring_buffer.hpp"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr std::uint32_t kRingMagic = 0x41564D52U;  // 'AVMR'

timespec make_abs_timeout(int timeout_ms) {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += static_cast<long>(timeout_ms % 1000) * 1'000'000L;
    if (ts.tv_nsec >= 1'000'000'000L) {
        ++ts.tv_sec;
        ts.tv_nsec -= 1'000'000'000L;
    }
    return ts;
}
}  // namespace

ShmRingBuffer::~ShmRingBuffer() {
    close();
}

bool ShmRingBuffer::create(const std::string& name, std::size_t elem_size, std::size_t capacity) {
    return map_common(name, elem_size, capacity, true);
}

bool ShmRingBuffer::open(const std::string& name, std::size_t elem_size, std::size_t capacity) {
    return map_common(name, elem_size, capacity, false);
}

bool ShmRingBuffer::map_common(const std::string& name,
                               std::size_t elem_size,
                               std::size_t capacity,
                               bool create_mode) {
    close();

    elem_size_ = elem_size;
    capacity_ = capacity;
    total_size_ = sizeof(RingHeader) + elem_size_ * capacity_;

    const int flags = O_RDWR | (create_mode ? O_CREAT : 0);
    fd_ = shm_open(name.c_str(), flags, 0666);
    if (fd_ < 0) {
        std::cerr << "[ShmRingBuffer] shm_open failed: " << name << " " << std::strerror(errno) << "\n";
        return false;
    }

    if (create_mode && ftruncate(fd_, static_cast<off_t>(total_size_)) != 0) {
        std::cerr << "[ShmRingBuffer] ftruncate failed: " << std::strerror(errno) << "\n";
        close();
        return false;
    }

    base_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (base_ == MAP_FAILED) {
        std::cerr << "[ShmRingBuffer] mmap failed: " << std::strerror(errno) << "\n";
        base_ = nullptr;
        close();
        return false;
    }

    header_ = static_cast<RingHeader*>(base_);
    if (create_mode || header_->magic != kRingMagic || header_->initialized != 1) {
        pthread_mutexattr_t mutex_attr{};
        pthread_condattr_t cond_attr{};
        pthread_mutexattr_init(&mutex_attr);
        pthread_condattr_init(&cond_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);

        pthread_mutex_init(&header_->mutex, &mutex_attr);
        pthread_cond_init(&header_->not_empty, &cond_attr);
        pthread_cond_init(&header_->not_full, &cond_attr);

        pthread_mutexattr_destroy(&mutex_attr);
        pthread_condattr_destroy(&cond_attr);

        header_->magic = kRingMagic;
        header_->elem_size = static_cast<std::uint32_t>(elem_size_);
        header_->capacity = static_cast<std::uint32_t>(capacity_);
        header_->head = 0;
        header_->tail = 0;
        header_->count = 0;
        header_->initialized = 1;
    }

    return true;
}

std::uint8_t* ShmRingBuffer::data_base() {
    return static_cast<std::uint8_t*>(base_) + sizeof(RingHeader);
}

bool ShmRingBuffer::push(const void* item, std::size_t elem_size) {
    if (header_ == nullptr || item == nullptr || elem_size != elem_size_) {
        return false;
    }

    pthread_mutex_lock(&header_->mutex);
    while (header_->count == header_->capacity) {
        pthread_cond_wait(&header_->not_full, &header_->mutex);
    }

    const std::uint64_t index = header_->tail % header_->capacity;
    std::memcpy(data_base() + index * elem_size_, item, elem_size_);
    ++header_->tail;
    ++header_->count;

    pthread_cond_signal(&header_->not_empty);
    pthread_mutex_unlock(&header_->mutex);
    return true;
}

bool ShmRingBuffer::pop(void* item, std::size_t elem_size, int timeout_ms) {
    if (header_ == nullptr || item == nullptr || elem_size != elem_size_) {
        return false;
    }

    pthread_mutex_lock(&header_->mutex);
    while (header_->count == 0) {
        int rc = 0;
        if (timeout_ms >= 0) {
            timespec ts = make_abs_timeout(timeout_ms);
            rc = pthread_cond_timedwait(&header_->not_empty, &header_->mutex, &ts);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&header_->mutex);
                return false;
            }
        } else {
            rc = pthread_cond_wait(&header_->not_empty, &header_->mutex);
        }

        if (rc != 0 && rc != ETIMEDOUT) {
            pthread_mutex_unlock(&header_->mutex);
            return false;
        }
    }

    const std::uint64_t index = header_->head % header_->capacity;
    std::memcpy(item, data_base() + index * elem_size_, elem_size_);
    ++header_->head;
    --header_->count;

    pthread_cond_signal(&header_->not_full);
    pthread_mutex_unlock(&header_->mutex);
    return true;
}

void ShmRingBuffer::close() {
    if (base_ != nullptr) {
        munmap(base_, total_size_);
        base_ = nullptr;
        header_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}
