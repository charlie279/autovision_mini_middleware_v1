/**
 * @file npu_stub_node.cpp
 * @brief NPU Stub 节点：读取 TensorMeta，模拟 NPU SDK init/run/release 和推理延迟。
 */
#include "avm_config.hpp"
#include "shared_status.hpp"
#include "shm_ring_buffer.hpp"
#include "tensor_meta.hpp"
#include "time_utils.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>

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

class NpuEngine {
public:
    bool init(const std::string& model_path) {
        model_path_ = model_path;
        std::cout << "[NpuEngine] init model=" << model_path_ << " (stub)\n";
        return true;
    }

    bool run(const TensorMeta& input, int fake_latency_ms, std::string& json_out) {
        const std::uint64_t start_ns = avm::now_ns();
        std::this_thread::sleep_for(std::chrono::milliseconds(fake_latency_ms));

        std::uniform_int_distribution<int> count_dist(0, 5);
        std::uniform_real_distribution<double> confidence_dist(0.50, 0.99);

        const double latency_ms = avm::ns_to_ms(avm::now_ns() - start_ns);
        std::ostringstream oss;
        oss << "{\"frame_id\":" << input.frame_id
            << ",\"object_count\":" << count_dist(rng_)
            << ",\"confidence\":" << confidence_dist(rng_)
            << ",\"npu_latency_ms\":" << latency_ms << "}";
        json_out = oss.str();
        return true;
    }

    void release() {
        std::cout << "[NpuEngine] release\n";
    }

private:
    std::string model_path_;
    std::mt19937 rng_{std::random_device{}()};
};
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const int frames = std::stoi(arg_value(argc, argv, "--frames", "120"));
    const int fake_latency_ms = std::stoi(arg_value(argc, argv, "--fake-latency", "8"));
    const std::string model_path = arg_value(argc, argv, "--model", "models/fake_model.tflite");

    std::filesystem::create_directories("logs");

    SharedStatus status;
    status.create_or_open(avm::kStatusName);

    ShmRingBuffer tensor_ring;
    if (!retry_open([&]() { return tensor_ring.open(avm::kTensorMetaRingName, sizeof(TensorMeta), avm::kRingCapacity); },
                    "tensor_meta_ring")) {
        return 1;
    }

    NpuEngine engine;
    engine.init(model_path);

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

        std::string result_json;
        engine.run(tensor, fake_latency_ms, result_json);
        result_file << result_json << "\n";

        latest_frame_id = tensor.frame_id;
        ++consumed;
        status.update_node("npu", consumed, latest_frame_id);

        if (consumed == 1 || consumed % 30 == 0) {
            std::cout << "[NPUStub] " << result_json << "\n";
        }
    }

    engine.release();
    std::cout << "[npu_stub_node] finished consumed=" << consumed << "\n";
    return 0;
}
