/**
 * @file shared_status.cpp
 * @brief 共享运行状态实现：各进程通过该共享内存发布 heartbeat、计数和安全状态。
 */
#include "shared_status.hpp"

#include "time_utils.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr std::uint32_t kStatusMagic = 0x41564D53U;  // 'AVMS'
}

SharedStatus::~SharedStatus() {
    close();
}

bool SharedStatus::create_or_open(const std::string& name) {
    return map_common(name, true);
}

bool SharedStatus::open_existing(const std::string& name) {
    return map_common(name, false);
}

bool SharedStatus::map_common(const std::string& name, bool create_mode) {
    close();

    bool created_by_this_process = false;
    if (create_mode) {
        fd_ = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
        if (fd_ >= 0) {
            created_by_this_process = true;
        } else if (errno == EEXIST) {
            fd_ = shm_open(name.c_str(), O_RDWR, 0666);
        }
    } else {
        fd_ = shm_open(name.c_str(), O_RDWR, 0666);
    }

    if (fd_ < 0) {
        std::cerr << "[SharedStatus] shm_open failed: " << name << " " << std::strerror(errno) << "\n";
        return false;
    }

    if (created_by_this_process && ftruncate(fd_, static_cast<off_t>(sizeof(RuntimeStatusBlock))) != 0) {
        std::cerr << "[SharedStatus] ftruncate failed: " << std::strerror(errno) << "\n";
        close();
        return false;
    }

    base_ = mmap(nullptr, sizeof(RuntimeStatusBlock), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (base_ == MAP_FAILED) {
        std::cerr << "[SharedStatus] mmap failed: " << std::strerror(errno) << "\n";
        base_ = nullptr;
        close();
        return false;
    }

    block_ = static_cast<RuntimeStatusBlock*>(base_);
    if (created_by_this_process || block_->magic != kStatusMagic || block_->initialized != 1) {
        pthread_mutexattr_t attr{};
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&block_->mutex, &attr);
        pthread_mutexattr_destroy(&attr);

        block_->magic = kStatusMagic;
        block_->desired_fps = 30;
        block_->safety_state = static_cast<std::uint32_t>(SafetyState::NORMAL);
        block_->global_error_code = static_cast<std::uint32_t>(ErrorCode::OK);
        std::snprintf(block_->safety_text, sizeof(block_->safety_text), "NORMAL");
        init_node(block_->media, "media");
        init_node(block_->preprocess, "preprocess");
        init_node(block_->npu, "npu");
        block_->initialized = 1;
    }

    return true;
}

void SharedStatus::init_node(NodeRuntimeStatus& node, const char* name) {
    node = NodeRuntimeStatus{};
    std::snprintf(node.name, sizeof(node.name), "%s", name);
    node.heartbeat_ns = avm::now_ns();
    node.error_code = static_cast<std::uint32_t>(ErrorCode::OK);
}

NodeRuntimeStatus* SharedStatus::select_node(const std::string& node_name) {
    if (block_ == nullptr) {
        return nullptr;
    }
    if (node_name == "media") {
        return &block_->media;
    }
    if (node_name == "preprocess") {
        return &block_->preprocess;
    }
    if (node_name == "npu") {
        return &block_->npu;
    }
    return nullptr;
}

void SharedStatus::update_node(const std::string& node_name,
                               std::uint64_t frame_count,
                               std::uint64_t latest_frame_id,
                               ErrorCode error_code,
                               std::uint64_t crc_errors,
                               std::uint64_t frame_jumps,
                               std::uint64_t alive_errors,
                               std::uint64_t queue_drops) {
    if (block_ == nullptr) {
        return;
    }

    pthread_mutex_lock(&block_->mutex);
    NodeRuntimeStatus* node = select_node(node_name);
    if (node != nullptr) {
        node->heartbeat_ns = avm::now_ns();
        node->frame_count = frame_count;
        node->latest_frame_id = latest_frame_id;
        node->error_code = static_cast<std::uint32_t>(error_code);
        node->crc_error_count = crc_errors;
        node->frame_jump_count = frame_jumps;
        node->alive_error_count = alive_errors;
        node->queue_drop_count = queue_drops;
    }
    pthread_mutex_unlock(&block_->mutex);
}

void SharedStatus::set_desired_fps(std::uint32_t fps) {
    if (block_ == nullptr) {
        return;
    }
    pthread_mutex_lock(&block_->mutex);
    block_->desired_fps = fps;
    pthread_mutex_unlock(&block_->mutex);
}

std::uint32_t SharedStatus::desired_fps() {
    if (block_ == nullptr) {
        return 30;
    }
    pthread_mutex_lock(&block_->mutex);
    const std::uint32_t fps = block_->desired_fps;
    pthread_mutex_unlock(&block_->mutex);
    return fps;
}

void SharedStatus::set_safety(SafetyState state, ErrorCode error_code, const std::string& text) {
    if (block_ == nullptr) {
        return;
    }
    pthread_mutex_lock(&block_->mutex);
    block_->safety_state = static_cast<std::uint32_t>(state);
    block_->global_error_code = static_cast<std::uint32_t>(error_code);
    std::snprintf(block_->safety_text, sizeof(block_->safety_text), "%s", text.c_str());
    pthread_mutex_unlock(&block_->mutex);
}

RuntimeStatusBlock SharedStatus::snapshot() {
    RuntimeStatusBlock copy{};
    if (block_ == nullptr) {
        return copy;
    }
    pthread_mutex_lock(&block_->mutex);
    copy = *block_;
    pthread_mutex_unlock(&block_->mutex);
    return copy;
}

void SharedStatus::close() {
    if (base_ != nullptr) {
        munmap(base_, sizeof(RuntimeStatusBlock));
        base_ = nullptr;
        block_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}
