/**
 * @file image_preprocess.hpp
 * @brief CPU fallback image preprocess helpers for RGB888 and YUYV input.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace avm {

std::uint8_t clamp_to_u8(int value);

struct ResizeIndexPlan {
    std::uint32_t src_width = 0;
    std::uint32_t src_height = 0;
    std::uint32_t src_stride_bytes = 0;
    std::uint32_t dst_width = 0;
    std::uint32_t dst_height = 0;
    std::vector<std::uint32_t> row_offsets;
    std::vector<std::uint32_t> rgb_x_offsets;
    std::vector<std::uint32_t> yuyv_pair_offsets;
    std::vector<std::uint8_t> yuyv_select_y1;
};

ResizeIndexPlan make_resize_index_plan(std::uint32_t src_width,
                                        std::uint32_t src_height,
                                        std::uint32_t src_stride_bytes,
                                        std::uint32_t dst_width,
                                        std::uint32_t dst_height);

void resize_rgb888_to_tensor(const std::uint8_t* rgb,
                             std::uint32_t src_width,
                             std::uint32_t src_height,
                             std::uint32_t src_stride_bytes,
                             std::uint32_t dst_width,
                             std::uint32_t dst_height,
                             float* tensor);

void resize_yuyv_to_tensor(const std::uint8_t* yuyv,
                           std::uint32_t src_width,
                           std::uint32_t src_height,
                           std::uint32_t src_stride_bytes,
                           std::uint32_t dst_width,
                           std::uint32_t dst_height,
                           float* tensor);

void resize_rgb888_to_tensor_plan(const std::uint8_t* rgb,
                                  const ResizeIndexPlan& plan,
                                  float* tensor);

void resize_yuyv_to_tensor_plan(const std::uint8_t* yuyv,
                                const ResizeIndexPlan& plan,
                                float* tensor);

void resize_nv12_to_tensor_plan(const std::uint8_t* nv12,
                                std::uint32_t src_width,
                                std::uint32_t src_height,
                                std::uint32_t src_stride_bytes,
                                std::uint32_t uv_stride_bytes,
                                const ResizeIndexPlan& plan,
                                float* tensor);

enum class PreprocessBackend : std::uint32_t {
    CPU_FALLBACK = 0,
    LIBYUV_COMPAT = 1
};

void preprocess_to_tensor(const std::uint8_t* data,
                          std::uint32_t format,
                          std::uint32_t src_width,
                          std::uint32_t src_height,
                          std::uint32_t src_stride_bytes,
                          PreprocessBackend backend,
                          const ResizeIndexPlan& plan,
                          float* tensor);

const char* preprocess_backend_name(PreprocessBackend backend);

}  // namespace avm
