/**
 * @file runtime_output.hpp
 * @brief SDK-style 推理 Runtime 输出结构。
 */
#pragma once

#include <cstdint>
#include <string>

struct RuntimeOutput {
    std::uint64_t frame_id = 0;
    int object_count = 0;
    double confidence = 0.0;
    double latency_ms = 0.0;
    std::string backend;
};