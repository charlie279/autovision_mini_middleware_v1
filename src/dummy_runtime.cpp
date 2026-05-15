/**
 * @file dummy_runtime.cpp
 * @brief Dummy Runtime 实现：模拟 SDK init/setInput/run/release 边界。
 */
#include "dummy_runtime.hpp"

#include "time_utils.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

bool DummyRuntime::init(const RuntimeConfig& config) {
    config_ = config;
    initialized_ = true;
    std::cout << "[DummyRuntime] init model=" << config_.model_path
              << " fake_latency_ms=" << config_.fake_latency_ms << "\n";
    return true;
}

bool DummyRuntime::setInput(const TensorMeta& meta, const void* data, std::size_t size) {
    if (!initialized_ || data == nullptr || size < meta.data_size) {
        std::cerr << "[DummyRuntime] invalid input size=" << size
                  << " expected=" << meta.data_size << "\n";
        return false;
    }
    latest_input_ = meta;

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    input_probe_ = 0;
    const std::size_t step = size / 16 + 1;
    for (std::size_t i = 0; i < size; i += step) {
        input_probe_ = input_probe_ * 131U + bytes[i];
    }
    has_input_ = true;
    return true;
}

bool DummyRuntime::run(RuntimeOutput& output) {
    if (!initialized_ || !has_input_) {
        return false;
    }

    const std::uint64_t start_ns = avm::now_ns();
    std::this_thread::sleep_for(std::chrono::milliseconds(config_.fake_latency_ms));

    std::uniform_int_distribution<int> count_dist(0, 5);
    std::uniform_real_distribution<double> confidence_dist(0.50, 0.99);

    output.frame_id = latest_input_.frame_id;
    output.object_count = count_dist(rng_);
    output.confidence = confidence_dist(rng_);
    output.latency_ms = avm::ns_to_ms(avm::now_ns() - start_ns);
    output.backend = config_.backend + "#" + std::to_string(input_probe_ & 0xFFFFU);
    return true;
}

void DummyRuntime::release() {
    std::cout << "[DummyRuntime] release\n";
    initialized_ = false;
    has_input_ = false;
}

RuntimeMetadata DummyRuntime::metadata() const {
    RuntimeMetadata m;
    m.backend = "dummy";
    m.model_path = config_.model_path;
    m.sdk_style = "dummy-runtime";
    m.inputs.push_back({"input0", {1, 320, 320, 3}, "float32", "NHWC", 0, 1.0F});
    m.outputs.push_back({"objects", {1, 6}, "float32", "NCHW", 0, 1.0F});
    return m;
}
