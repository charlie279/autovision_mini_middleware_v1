/**
 * @file runtime_metadata.hpp
 * @brief RKNN-like runtime metadata and IO query mock schema.
 */
#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

struct TensorInfo {
    std::string name;
    std::vector<std::uint32_t> shape;
    std::string dtype;
    std::string layout;
    std::uint32_t zero_point = 0;
    float scale = 1.0F;
};

struct RuntimeMetadata {
    std::string backend;
    std::string model_path;
    std::string sdk_style;
    std::vector<TensorInfo> inputs;
    std::vector<TensorInfo> outputs;
    int last_error = 0;

    std::string to_json() const;
};

