/**
 * @file dummy_runtime.hpp
 * @brief Ubuntu/x86 可运行的 Dummy 推理 Runtime。
 */
#pragma once

#include "inference_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <random>

class DummyRuntime final : public InferenceRuntime {
public:
    bool init(const RuntimeConfig& config) override;
    bool setInput(const TensorMeta& meta, const void* data, std::size_t size) override;
    bool run(RuntimeOutput& output) override;
    void release() override;
    RuntimeMetadata metadata() const override;

private:
    RuntimeConfig config_;
    TensorMeta latest_input_{};
    std::uint32_t input_probe_ = 0;
    bool initialized_ = false;
    bool has_input_ = false;
    std::mt19937 rng_{std::random_device{}()};
};
