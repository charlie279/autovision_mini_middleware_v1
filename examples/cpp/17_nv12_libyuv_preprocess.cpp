
#include "avm_config.hpp"
#include "image_preprocess.hpp"
#include "latency_profiler.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    constexpr std::uint32_t w = 640;
    constexpr std::uint32_t h = 480;
    std::vector<std::uint8_t> nv12(static_cast<std::size_t>(w) * h * 3U / 2U);
    std::uint8_t* y = nv12.data();
    std::uint8_t* uv = nv12.data() + static_cast<std::size_t>(w) * h;
    for (std::uint32_t row = 0; row < h; ++row) {
        for (std::uint32_t col = 0; col < w; ++col) {
            y[static_cast<std::size_t>(row) * w + col] = static_cast<std::uint8_t>((row + col) & 0xFFU);
        }
    }
    for (std::size_t i = 0; i < static_cast<std::size_t>(w) * h / 2U; i += 2) {
        uv[i + 0] = 128;
        uv[i + 1] = 128;
    }
    auto plan = avm::make_resize_index_plan(w, h, w, avm::kTensorWidth, avm::kTensorHeight);
    std::vector<float> tensor(static_cast<std::size_t>(avm::kTensorWidth) * avm::kTensorHeight * avm::kTensorChannels);
    LatencyProfiler profiler;
    for (int i = 0; i < 40; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        avm::preprocess_to_tensor(nv12.data(), avm::kFormatNv12, w, h, w,
                                  avm::PreprocessBackend::LIBYUV_COMPAT, plan, tensor.data());
        const auto t1 = std::chrono::steady_clock::now();
        profiler.add(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    std::cout << "[17_nv12_libyuv_preprocess] backend="
              << avm::preprocess_backend_name(avm::PreprocessBackend::LIBYUV_COMPAT)
              << " input_format=NV12 input_bytes=" << nv12.size()
              << " tensor0=" << tensor[0]
              << " mean=" << profiler.mean()
              << " p95=" << profiler.percentile(95.0) << "\n";
    return 0;
}
