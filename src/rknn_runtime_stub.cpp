/**
 * @file rknn_runtime_stub.cpp
 * @brief RKNN Runtime 占位实现：用于保持节点层与未来板端 RKNN SDK 的接口边界一致。
 */
#include "rknn_runtime_stub.hpp"

#include "time_utils.hpp"

#include <chrono>
#include <iostream>
#include <thread>

bool RknnRuntimeStub::init(const RuntimeConfig& config) {
    config_ = config;
    initialized_ = true;
    std::cout << "[RknnRuntimeStub] init model=" << config_.model_path
              << " (stub only, no RKNN SDK linked)\n";
    return true;
}

bool RknnRuntimeStub::setInput(const TensorMeta& meta, const void* data, std::size_t size) {
    if (!initialized_ || data == nullptr || size < meta.data_size) {
        std::cerr << "[RknnRuntimeStub] invalid input size=" << size
                  << " expected=" << meta.data_size << "\n";
        return false;
    }
    latest_input_ = meta;
    has_input_ = true;
    return true;
}

bool RknnRuntimeStub::run(RuntimeOutput& output) {
    if (!initialized_ || !has_input_) {
        return false;
    }
    const std::uint64_t start_ns = avm::now_ns();
    std::this_thread::sleep_for(std::chrono::milliseconds(config_.fake_latency_ms));

    output.frame_id = latest_input_.frame_id;
    output.object_count = static_cast<int>(latest_input_.frame_id % 4U);
    output.confidence = 0.75;
    output.latency_ms = avm::ns_to_ms(avm::now_ns() - start_ns);
    output.backend = "rknn_stub";
    return true;
}

void RknnRuntimeStub::release() {
    std::cout << "[RknnRuntimeStub] release\n";
    initialized_ = false;
    has_input_ = false;
}