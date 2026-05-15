/**
 * @file rknn_runtime_stub.hpp
 * @brief RKNN Runtime 占位 backend：不链接真实 RKNN SDK，只保留 SDK-style 接口边界。
 */
#pragma once

#include "inference_runtime.hpp"

class RknnRuntimeStub final : public InferenceRuntime {
public:
    bool init(const RuntimeConfig& config) override;
    bool setInput(const TensorMeta& meta, const void* data, std::size_t size) override;
    bool run(RuntimeOutput& output) override;
    void release() override;
    RuntimeMetadata metadata() const override;
    int last_error() const override { return last_error_; }

private:
    RuntimeConfig config_;
    TensorMeta latest_input_{};
    bool initialized_ = false;
    bool has_input_ = false;
    int last_error_ = 0;
};