/**
 * @file shared_status.hpp
 * @brief 运行状态共享内存：保存各节点 heartbeat、帧计数、错误计数和全局 Safety 状态。
 */
#pragma once

#include "safety_status.hpp"

#include <cstdint>
#include <pthread.h>
#include <string>

struct NodeRuntimeStatus {
    char name[32]{};
    std::uint64_t heartbeat_ns = 0;
    std::uint64_t frame_count = 0;
    std::uint64_t latest_frame_id = 0;
    std::uint64_t crc_error_count = 0;
    std::uint64_t frame_jump_count = 0;
    std::uint64_t alive_error_count = 0;
    std::uint64_t queue_drop_count = 0;
    std::uint32_t error_code = 0;
    std::uint32_t reserved = 0;
};

struct RuntimeStatusBlock {
    std::uint32_t magic = 0;
    std::uint32_t initialized = 0;
    pthread_mutex_t mutex{};
    std::uint32_t desired_fps = 30;
    std::uint32_t safety_state = 0;
    std::uint32_t global_error_code = 0;
    char safety_text[128]{};
    NodeRuntimeStatus media;
    NodeRuntimeStatus preprocess;
    NodeRuntimeStatus npu;
};

class SharedStatus {
public:
    SharedStatus() = default;
    ~SharedStatus();

    bool create_or_open(const std::string& name);
    bool open_existing(const std::string& name);
    void update_node(const std::string& node_name,
                     std::uint64_t frame_count,
                     std::uint64_t latest_frame_id,
                     ErrorCode error_code = ErrorCode::OK,
                     std::uint64_t crc_errors = 0,
                     std::uint64_t frame_jumps = 0,
                     std::uint64_t alive_errors = 0,
                     std::uint64_t queue_drops = 0);
    void set_desired_fps(std::uint32_t fps);
    std::uint32_t desired_fps();
    void set_safety(SafetyState state, ErrorCode error_code, const std::string& text);
    RuntimeStatusBlock snapshot();
    void close();

private:
    bool map_common(const std::string& name, bool create_mode);
    static void init_node(NodeRuntimeStatus& node, const char* name);
    NodeRuntimeStatus* select_node(const std::string& node_name);

    int fd_ = -1;
    void* base_ = nullptr;
    RuntimeStatusBlock* block_ = nullptr;
};