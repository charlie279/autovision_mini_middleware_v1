
#include "detection_result.hpp"
#include "dummy_runtime.hpp"
#include "rknn_runtime_stub.hpp"
#include "runtime_config.hpp"
#include "runtime_metadata.hpp"
#include "tensor_meta.hpp"
#include "time_utils.hpp"

#include <iostream>
#include <vector>

int main() {
    RuntimeConfig cfg;
    cfg.backend = "rknn_stub";
    cfg.model_path = "models/mock.rknn";
    cfg.fake_latency_ms = 1;
    RknnRuntimeStub runtime;
    runtime.init(cfg);
    std::cout << "[18_runtime_metadata_detection] metadata=" << runtime.metadata().to_json() << "\n";

    TensorMeta meta{};
    meta.frame_id = 7;
    meta.width = 320;
    meta.height = 320;
    meta.channels = 3;
    meta.data_type = 1;
    meta.data_size = 320 * 320 * 3 * sizeof(float);
    std::vector<float> tensor(320 * 320 * 3, 0.5F);
    runtime.setInput(meta, tensor.data(), meta.data_size);
    RuntimeOutput output;
    runtime.run(output);
    DetectionResult dr = make_stub_detection_result(output, avm::now_ns());
    std::cout << "[18_runtime_metadata_detection] detection=" << dr.to_json() << "\n";
    runtime.release();
    return 0;
}
