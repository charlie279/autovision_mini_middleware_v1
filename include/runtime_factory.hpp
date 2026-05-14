/**
 * @file runtime_factory.hpp
 * @brief 推理 Runtime 工厂函数。
 */
#pragma once

#include "inference_runtime.hpp"

#include <memory>
#include <string>

std::unique_ptr<InferenceRuntime> create_runtime(const std::string& backend);
