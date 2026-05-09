/**
 * @file sensor_frame.hpp
 * @brief 统一传感器帧结构：用于 FileAdapter、LidarSimAdapter 和未来 CameraAdapter 输出统一数据。
 */
#pragma once

#include <cstdint>
#include <vector>

struct SensorFrame {
    std::uint64_t frame_id = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint32_t sensor_type = 0;  // 0=camera, 1=lidar_sim, 2=file
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t format = 0;
    std::uint32_t data_size = 0;
    std::vector<std::uint8_t> data;
};
