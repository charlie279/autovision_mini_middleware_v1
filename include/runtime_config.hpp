/**
 * @file runtime_config.hpp
 * @brief SDK-style 推理 Runtime 配置结构。
 */
#pragma once

#include <string>

struct RuntimeConfig {
    std::string backend = "dummy";        // dummy / rknn_stub
    std::string model_path = "models/fake_model.tflite";
    int fake_latency_ms = 8;
};
