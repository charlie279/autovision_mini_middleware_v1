/**
 * @file runtime_metadata.cpp
 * @brief RuntimeMetadata JSON serialization.
 */
#include "runtime_metadata.hpp"

namespace {
std::string shape_json(const std::vector<std::uint32_t>& shape) {
    std::ostringstream oss;
    oss << '[';
    for (std::size_t i = 0; i < shape.size(); ++i) {
        if (i != 0) oss << ',';
        oss << shape[i];
    }
    oss << ']';
    return oss.str();
}

void append_tensor(std::ostringstream& oss, const TensorInfo& t) {
    oss << "{\"name\":\"" << t.name << "\",\"shape\":" << shape_json(t.shape)
        << ",\"dtype\":\"" << t.dtype << "\",\"layout\":\"" << t.layout
        << "\",\"zero_point\":" << t.zero_point << ",\"scale\":" << t.scale << '}';
}
}

std::string RuntimeMetadata::to_json() const {
    std::ostringstream oss;
    oss << "{\"backend\":\"" << backend << "\",\"model_path\":\"" << model_path
        << "\",\"sdk_style\":\"" << sdk_style << "\",\"last_error\":" << last_error;
    oss << ",\"inputs\":[";
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        if (i != 0) oss << ',';
        append_tensor(oss, inputs[i]);
    }
    oss << "],\"outputs\":[";
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        if (i != 0) oss << ',';
        append_tensor(oss, outputs[i]);
    }
    oss << "]}";
    return oss.str();
}
