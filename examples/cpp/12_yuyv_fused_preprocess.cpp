/**
 * @file 12_yuyv_fused_preprocess.cpp
 * @brief 示例：构造合成 YUYV 帧，验证 YUYV->RGB->resize->normalize 融合前处理函数。
 */
#include "avm_config.hpp"
#include "image_preprocess.hpp"
#include "latency_profiler.hpp"
#include "time_utils.hpp"

#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

namespace {
std::vector<std::uint8_t> make_synthetic_yuyv(std::uint32_t width, std::uint32_t height) {
    std::vector<std::uint8_t> yuyv(static_cast<std::size_t>(width) * height * 2U);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; x += 2) {
            const std::size_t idx = (static_cast<std::size_t>(y) * width + x) * 2U;
            yuyv[idx + 0] = static_cast<std::uint8_t>(16U + (x * 219U / width));
            yuyv[idx + 1] = static_cast<std::uint8_t>(128U + (y % 32U));
            yuyv[idx + 2] = static_cast<std::uint8_t>(16U + ((x + 1U) * 219U / width));
            yuyv[idx + 3] = static_cast<std::uint8_t>(128U + (x % 32U));
        }
    }
    return yuyv;
}
}  // namespace

int main() {
    const std::uint32_t src_width = avm::kDefaultWidth;
    const std::uint32_t src_height = avm::kDefaultHeight;
    const std::uint32_t src_stride = src_width * 2U;
    auto yuyv = make_synthetic_yuyv(src_width, src_height);
    std::vector<float> tensor(static_cast<std::size_t>(avm::kTensorWidth) * avm::kTensorHeight * avm::kTensorChannels);

    LatencyProfiler profiler;
    constexpr int kLoops = 50;
    for (int i = 0; i < kLoops; ++i) {
        const std::uint64_t t0 = avm::now_ns();
        avm::resize_yuyv_to_tensor(yuyv.data(), src_width, src_height, src_stride,
                                   avm::kTensorWidth, avm::kTensorHeight, tensor.data());
        profiler.add(avm::ns_to_ms(avm::now_ns() - t0));
    }

    const double sum = std::accumulate(tensor.begin(), tensor.end(), 0.0);
    const double mean_value = sum / static_cast<double>(tensor.size());

    std::cout << "[12_yuyv_fused_preprocess] input_format=YUYV"
              << " input_bytes=" << yuyv.size()
              << " output_tensor_bytes=" << tensor.size() * sizeof(float)
              << " loops=" << kLoops
              << " mean_ms=" << profiler.mean()
              << " p95_ms=" << profiler.percentile(95.0)
              << " tensor_mean=" << mean_value << "\n";
    return 0;
}