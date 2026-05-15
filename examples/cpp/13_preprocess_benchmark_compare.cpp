/**
 * @file 13_preprocess_benchmark_compare.cpp
 * @brief 示例：合成 RGB888 与 YUYV 输入，输出可视化脚本可解析的性能对比 CSV。
 */
#include "avm_config.hpp"
#include "image_preprocess.hpp"
#include "latency_profiler.hpp"
#include "time_utils.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> make_synthetic_rgb(std::uint32_t width, std::uint32_t height) {
    std::vector<std::uint8_t> rgb(static_cast<std::size_t>(width) * height * 3U);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t idx = (static_cast<std::size_t>(y) * width + x) * 3U;
            rgb[idx + 0] = static_cast<std::uint8_t>(x % 256U);
            rgb[idx + 1] = static_cast<std::uint8_t>(y % 256U);
            rgb[idx + 2] = static_cast<std::uint8_t>((x + y) % 256U);
        }
    }
    return rgb;
}

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

double tensor_mean(const std::vector<float>& tensor) {
    return std::accumulate(tensor.begin(), tensor.end(), 0.0) / static_cast<double>(tensor.size());
}

double max_abs_diff(const std::vector<float>& a, const std::vector<float>& b) {
    double v = 0.0;
    for (std::size_t i = 0; i < a.size() && i < b.size(); ++i) {
        v = std::max(v, std::abs(static_cast<double>(a[i] - b[i])));
    }
    return v;
}
}  // namespace

int main(int argc, char** argv) {
    const int loops = (argc > 1) ? std::stoi(argv[1]) : 100;
    const std::uint32_t src_width = avm::kDefaultWidth;
    const std::uint32_t src_height = avm::kDefaultHeight;

    const auto rgb = make_synthetic_rgb(src_width, src_height);
    const auto yuyv = make_synthetic_yuyv(src_width, src_height);
    std::vector<float> tensor(static_cast<std::size_t>(avm::kTensorWidth) * avm::kTensorHeight * avm::kTensorChannels);
    std::vector<float> tensor_ref(tensor.size());

    const auto rgb_plan = avm::make_resize_index_plan(src_width, src_height, src_width * 3U,
                                                      avm::kTensorWidth, avm::kTensorHeight);
    const auto yuyv_plan = avm::make_resize_index_plan(src_width, src_height, src_width * 2U,
                                                       avm::kTensorWidth, avm::kTensorHeight);

    std::filesystem::create_directories("examples/logs");
    const std::string csv_path = "examples/logs/preprocess_benchmark_compare.csv";
    std::ofstream csv(csv_path, std::ios::out | std::ios::trunc);
    csv << "case,input_format,input_bytes,stride_bytes,loop,preprocess_ms,tensor_mean\n";

    LatencyProfiler rgb_profiler;
    for (int i = 0; i < loops; ++i) {
        const std::uint64_t t0 = avm::now_ns();
        avm::resize_rgb888_to_tensor(rgb.data(), src_width, src_height, src_width * 3U,
                                     avm::kTensorWidth, avm::kTensorHeight, tensor.data());
        const double ms = avm::ns_to_ms(avm::now_ns() - t0);
        rgb_profiler.add(ms);
        csv << "rgb_baseline_div,RGB888," << rgb.size() << ',' << src_width * 3U << ','
            << i << ',' << ms << ',' << tensor_mean(tensor) << '\n';
    }
    tensor_ref = tensor;

    LatencyProfiler rgb_plan_profiler;
    for (int i = 0; i < loops; ++i) {
        const std::uint64_t t0 = avm::now_ns();
        avm::resize_rgb888_to_tensor_plan(rgb.data(), rgb_plan, tensor.data());
        const double ms = avm::ns_to_ms(avm::now_ns() - t0);
        rgb_plan_profiler.add(ms);
        csv << "rgb_plan_index,RGB888," << rgb.size() << ',' << src_width * 3U << ','
            << i << ',' << ms << ',' << tensor_mean(tensor) << '\n';
    }
    const double rgb_diff = max_abs_diff(tensor_ref, tensor);

    LatencyProfiler yuyv_profiler;
    for (int i = 0; i < loops; ++i) {
        const std::uint64_t t0 = avm::now_ns();
        avm::resize_yuyv_to_tensor(yuyv.data(), src_width, src_height, src_width * 2U,
                                   avm::kTensorWidth, avm::kTensorHeight, tensor.data());
        const double ms = avm::ns_to_ms(avm::now_ns() - t0);
        yuyv_profiler.add(ms);
        csv << "yuyv_fused_div,YUYV," << yuyv.size() << ',' << src_width * 2U << ','
            << i << ',' << ms << ',' << tensor_mean(tensor) << '\n';
    }
    tensor_ref = tensor;

    LatencyProfiler yuyv_plan_profiler;
    for (int i = 0; i < loops; ++i) {
        const std::uint64_t t0 = avm::now_ns();
        avm::resize_yuyv_to_tensor_plan(yuyv.data(), yuyv_plan, tensor.data());
        const double ms = avm::ns_to_ms(avm::now_ns() - t0);
        yuyv_plan_profiler.add(ms);
        csv << "yuyv_plan_index,YUYV," << yuyv.size() << ',' << src_width * 2U << ','
            << i << ',' << ms << ',' << tensor_mean(tensor) << '\n';
    }
    const double yuyv_diff = max_abs_diff(tensor_ref, tensor);

    std::cout << "[13_preprocess_benchmark_compare] loops=" << loops << "\n";
    std::cout << "[13_preprocess_benchmark_compare] rgb_input_bytes=" << rgb.size()
              << " div_mean_ms=" << rgb_profiler.mean()
              << " plan_mean_ms=" << rgb_plan_profiler.mean()
              << " plan_max_abs_diff=" << rgb_diff << "\n";
    std::cout << "[13_preprocess_benchmark_compare] yuyv_input_bytes=" << yuyv.size()
              << " div_mean_ms=" << yuyv_profiler.mean()
              << " plan_mean_ms=" << yuyv_plan_profiler.mean()
              << " plan_max_abs_diff=" << yuyv_diff << "\n";
    std::cout << "[13_preprocess_benchmark_compare] csv=" << csv_path << "\n";
    return (rgb_diff <= 1e-6 && yuyv_diff <= 1e-6) ? 0 : 2;
}