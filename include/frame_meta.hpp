/**
 * @file frame_meta.hpp
 * @brief 图像帧元数据结构：Ring 中只传元数据，图像 payload 放在 POSIX SHM FramePool 中。
 */
#pragma once

#include <cstdint>

struct FrameMeta {
    std::uint64_t frame_id = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint32_t sensor_type = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t format = 0;
    std::uint32_t data_size = 0;
    std::uint32_t stride_bytes = 0;
    std::uint32_t buffer_index = 0;
    std::uint32_t crc32 = 0;
    std::uint32_t alive_counter = 0;
    std::uint32_t status = 0;
};