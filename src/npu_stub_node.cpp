/**
 * @file npu_stub_node.cpp
 * @brief Runtime 推理节点：读取 TensorMeta + Tensor payload，通过 SDK-style InferenceRuntime backend 模拟推理。
 */
#include "avm_config.hpp"
#include "runtime_config.hpp"
#include "runtime_factory.hpp"
#include "runtime_output.hpp"
#include "shared_status.hpp"
#include "shm_frame_pool.hpp"
#include "shm_ring_buffer.hpp"
#include "tensor_meta.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {
std::string arg_value(int argc, char** argv, const std::string& key, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return default_value;
}

template <typename OpenFunc>
bool retry_open(OpenFunc&& open_func, const char* name, int retry_count = 80) {
    for (int i = 0; i < retry_count; ++i) {
        if (open_func()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cerr << "[npu_stub_node] failed to open " << name << "\n";
    return false;
}

std::string to_json(const RuntimeOutput& output) {
    std::ostringstream oss;
    oss << "{\"frame_id\":" << output.frame_id
        << ",\"object_count\":" << output.object_count
        << ",\"confidence\":" << output.confidence
        << ",\"npu_latency_ms\":" << output.latency_ms
        << ",\"backend\":\"" << output.backend << "\"}";
    return oss.str();
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const int frames = std::stoi(arg_value(argc, argv, "--frames", "120"));
    RuntimeConfig runtime_config;
    runtime_config.backend = arg_value(argc, argv, "--backend", "dummy");
    runtime_config.fake_latency_ms = std::stoi(arg_value(argc, argv, "--fake-latency", "8"));
    runtime_config.model_path = arg_value(argc, argv, "--model", "models/fake_model.tflite");

    std::filesystem::create_directories("logs");

    SharedStatus status;
    status.create_or_open(avm::kStatusName);

    ShmFramePool tensor_pool;
    if (!retry_open([&]() { return tensor_pool.open(avm::kTensorPoolName, avm::kTensorPoolCount, avm::kTensorBufferSize); },
                    "tensor_pool")) {
        return 1;
    }

    ShmRingBuffer tensor_ring;
    if (!retry_open([&]() { return tensor_ring.open(avm::kTensorMetaRingName, sizeof(TensorMeta), avm::kRingCapacity); },
                    "tensor_meta_ring")) {
        return 2;
    }

    std::unique_ptr<InferenceRuntime> runtime = create_runtime(runtime_config.backend);
    if (!runtime || !runtime->init(runtime_config)) {
        std::cerr << "[npu_stub_node] runtime init failed backend=" << runtime_config.backend << "\n";
        return 3;
    }

    std::ofstream result_file("logs/npu_results.jsonl", std::ios::out | std::ios::trunc);
    std::uint64_t consumed = 0;
    std::uint64_t latest_frame_id = 0;

    for (int i = 0; i < frames; ++i) {
        TensorMeta tensor;
        if (!tensor_ring.pop(&tensor, sizeof(tensor), 5000)) {
            std::cerr << "[npu_stub_node] timeout waiting TensorMeta\n";
            status.update_node("npu", consumed, latest_frame_id, ErrorCode::HEARTBEAT_TIMEOUT);
            continue;
        }

        if (tensor.data_size > avm::kTensorBufferSize) {
            std::cerr << "[npu_stub_node] tensor too large frame_id=" << tensor.frame_id
                      << " size=" << tensor.data_size << "\n";
            status.update_node("npu", consumed, latest_frame_id, ErrorCode::LATENCY_OVER_THRESHOLD);
            continue;
        }

        std::vector<std::uint8_t> tensor_payload(tensor.data_size);
        if (!tensor_pool.read(tensor.buffer_index, tensor_payload.data(), tensor_payload.size())) {
            std::cerr << "[npu_stub_node] tensor_pool.read failed frame_id=" << tensor.frame_id << "\n";
            status.update_node("npu", consumed, latest_frame_id, ErrorCode::HEARTBEAT_TIMEOUT);
            continue;
        }

        if (!runtime->setInput(tensor, tensor_payload.data(), tensor_payload.size())) {
            std::cerr << "[npu_stub_node] runtime setInput failed frame_id=" << tensor.frame_id << "\n";
            continue;
        }

        RuntimeOutput output;
        if (!runtime->run(output)) {
            std::cerr << "[npu_stub_node] runtime run failed frame_id=" << tensor.frame_id << "\n";
            continue;
        }

        const std::string result_json = to_json(output);
        result_file << result_json << "\n";

        latest_frame_id = tensor.frame_id;
        ++consumed;
        status.update_node("npu", consumed, latest_frame_id);

        if (consumed == 1 || consumed % 30 == 0) {
            std::cout << "[RuntimeNode] " << result_json << "\n";
        }
    }

    runtime->release();
    std::cout << "[npu_stub_node] finished consumed=" << consumed
              << " backend=" << runtime_config.backend << "\n";
    return 0;
}
