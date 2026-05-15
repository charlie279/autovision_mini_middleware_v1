/**
 * @file safety_status.hpp
 * @brief 安全状态与错误码定义：用于 Safety Monitor 和控制面状态查询。
 */
#pragma once

#include <cstdint>

enum class SafetyState : std::uint32_t {
    NORMAL = 0,
    DEGRADED = 1,
    FAULT = 2,
    RECOVERING = 3
};

enum class ErrorCode : std::uint32_t {
    OK = 0,
    FRAME_ID_JUMP = 1,
    CRC_ERROR = 2,
    HEARTBEAT_TIMEOUT = 3,
    LATENCY_OVER_THRESHOLD = 4
};

inline const char* safety_state_to_string(SafetyState state) {
    switch (state) {
        case SafetyState::NORMAL:
            return "NORMAL";
        case SafetyState::DEGRADED:
            return "DEGRADED";
        case SafetyState::FAULT:
            return "FAULT";
        case SafetyState::RECOVERING:
            return "RECOVERING";
        default:
            return "UNKNOWN";
    }
}