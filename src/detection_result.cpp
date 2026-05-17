/**
 * @file detection_result.cpp
 * @brief DetectionResult schema and deterministic algorithm stub helpers.
 */
#include "detection_result.hpp"

#include <sstream>

std::string DetectionResult::to_json() const {
    std::ostringstream oss;
    oss << "{\"frame_id\":" << frame_id
        << ",\"timestamp_ns\":" << timestamp_ns
        << ",\"source_backend\":\"" << source_backend << "\",\"detections\":[";
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const auto& b = boxes[i];
        if (i != 0) oss << ',';
        oss << "{\"x1\":" << b.x1 << ",\"y1\":" << b.y1
            << ",\"x2\":" << b.x2 << ",\"y2\":" << b.y2
            << ",\"score\":" << b.score
            << ",\"class_id\":" << b.class_id
            << ",\"label\":\"" << b.label << "\"}";
    }
    oss << "]}";
    return oss.str();
}

DetectionResult make_stub_detection_result(const RuntimeOutput& output, std::uint64_t timestamp_ns) {
    DetectionResult result;
    result.frame_id = output.frame_id;
    result.timestamp_ns = timestamp_ns;
    result.source_backend = output.backend;
    const int count = output.object_count < 0 ? 0 : output.object_count;
    for (int i = 0; i < count; ++i) {
        DetectionBox box;
        const float base = static_cast<float>((output.frame_id + static_cast<std::uint64_t>(i) * 13U) % 100U) / 100.0F;
        box.x1 = 0.05F + base * 0.35F;
        box.y1 = 0.08F + base * 0.25F;
        box.x2 = box.x1 + 0.20F;
        box.y2 = box.y1 + 0.18F;
        box.score = static_cast<float>(output.confidence) - static_cast<float>(i) * 0.03F;
        if (box.score < 0.01F) box.score = 0.01F;
        box.class_id = static_cast<std::uint32_t>(i % 3);
        box.label = (box.class_id == 0) ? "vehicle" : (box.class_id == 1 ? "pedestrian" : "cyclist");
        result.boxes.push_back(box);
    }
    return result;
}

