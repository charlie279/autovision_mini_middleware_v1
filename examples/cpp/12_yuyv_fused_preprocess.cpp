/**
 * @file 12_yuyv_fused_preprocess.cpp
 * @brief Validate fused YUYV->RGB resize->normalize CPU fallback path without a real camera.
 */
#include "avm_config.hpp"
#include "image_preprocess.hpp"
#include "latency_profiler.hpp"
#include "time_utils.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
std::vector<std::uint8_t> make_test_yuyv(std::uint32_t width, std::uint32_t height) {
    std::vector<std::uint8_t> yuyv(static_cast<std::size_t>(width) * height * 2U);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x + 1 < width; x += 2) {
            const std::size_t idx = (static_cast<std::size_t>(y) * width + x) * 2U;
            yuyv[idx + 0] = static_cast<std::uint8_t>((x + y) & 0xFFU);       // Y0
            yuyv[idx + 1] = 128;                                             // U
            yuyv[idx + 2] = static_cast<std::uint8_t>((x + y + 32U) & 0xFFU); // Y1
            yuyv[idx + 3] = 128;                                             // V
        }
    }
    return yuyv;
}
}  // namespace

int main() {
    constexpr std::uint32_t src_width = 640;
    constexpr std::uint32_t src_height = 480;
    constexpr std::uint32_t dst_width = avm::kTensorWidth;
    constexpr std::uint32_t dst_height = avm::kTensorHeight;
    constexpr std::uint32_t stride = src_width * 2U;

    const auto yuyv = make_test_yuyv(src_width, src_height);
    std::vector<float> tensor(static_cast<std::size_t>(dst_width) * dst_height * avm::kTensorChannels);

    LatencyProfiler profiler;
    for (int i = 0; i < 30; ++i) {
        const std::uint64_t t0 = avm::now_ns();
        avm::resize_yuyv_to_tensor(yuyv.data(), src_width, src_height, stride,
                                   dst_width, dst_height, tensor.data());
        profiler.add(avm::ns_to_ms(avm::now_ns() - t0));
    }

    std::cout << "[12_yuyv_fused_preprocess] input_format=YUYV"
              << " input_bytes=" << yuyv.size()
              << " output_tensor_bytes=" << tensor.size() * sizeof(float)
              << " mean=" << profiler.mean() << "ms"
              << " p95=" << profiler.percentile(95.0) << "ms"
              << " first_pixel=[" << tensor[0] << "," << tensor[1] << "," << tensor[2] << "]\n";
    return 0;
}
