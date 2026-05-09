/**
 * @file tensor_meta.hpp
 * @brief 前处理输出 Tensor 的元数据结构：NPU Stub 通过该结构获取 Tensor 索引和尺寸信息。
 */
#pragma once

#include <cstdint>

struct TensorMeta {
    std::uint64_t frame_id = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = 0;
    std::uint32_t data_type = 0;  // 0=uint8, 1=float32
    std::uint32_t data_size = 0;
    std::uint32_t buffer_index = 0;
};
