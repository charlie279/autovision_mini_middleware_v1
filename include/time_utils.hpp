/**
 * @file time_utils.hpp
 * @brief 时间工具：提供单调时钟纳秒时间戳和纳秒到毫秒的转换。
 */
#pragma once

#include <chrono>
#include <cstdint>

namespace avm {

inline std::uint64_t now_ns() {
    using Clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count());
}

inline double ns_to_ms(std::uint64_t ns) {
    return static_cast<double>(ns) / 1'000'000.0;
}

}  // namespace avm
