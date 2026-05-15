/**
 * @file image_preprocess.hpp
 * @brief CPU fallback image preprocess helpers for RGB888 and YUYV input.
 */
#pragma once

#include <cstdint>
#include <cstddef>

namespace avm {

std::uint8_t clamp_to_u8(int value);

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

}  // namespace avm
