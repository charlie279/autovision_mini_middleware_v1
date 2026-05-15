/**
 * @file inference_runtime.hpp
 * @brief 推理 Runtime 抽象接口：用于把 npu_stub_node 从固定 NpuEngine 改为可替换 backend。
 */
#pragma once

#include "runtime_config.hpp"
#include "runtime_output.hpp"
#include "runtime_metadata.hpp"
#include "tensor_meta.hpp"

#include <cstddef>

class InferenceRuntime {
public:
    virtual bool init(const RuntimeConfig& config) = 0;
    virtual bool setInput(const TensorMeta& meta, const void* data, std::size_t size) = 0;
    virtual bool run(RuntimeOutput& output) = 0;
    virtual void release() = 0;
    virtual RuntimeMetadata metadata() const { return RuntimeMetadata{}; }
    virtual int last_error() const { return 0; }
    virtual ~InferenceRuntime() = default;
};