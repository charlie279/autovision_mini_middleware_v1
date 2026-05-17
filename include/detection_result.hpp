/**
 * @file detection_result.hpp
 * @brief DetectionResult schema for algorithm platform stub output.
 */
#pragma once

#include "runtime_output.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct DetectionBox {
    float x1 = 0.0F;
    float y1 = 0.0F;
    float x2 = 0.0F;
    float y2 = 0.0F;
    float score = 0.0F;
    std::uint32_t class_id = 0;
    std::string label = "object";
};

struct DetectionResult {
    std::uint64_t frame_id = 0;
    std::uint64_t timestamp_ns = 0;
    std::string source_backend;
    std::vector<DetectionBox> boxes;

    std::string to_json() const;
};

DetectionResult make_stub_detection_result(const RuntimeOutput& output, std::uint64_t timestamp_ns);
