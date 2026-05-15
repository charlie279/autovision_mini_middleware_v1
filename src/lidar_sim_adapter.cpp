/**
 * @file lidar_sim_adapter.cpp
 * @brief 模拟雷达输入实现：生成 float32 距离数组，验证 SensorAdapter 抽象层。
 */
#include "lidar_sim_adapter.hpp"

#include "avm_config.hpp"
#include "time_utils.hpp"

#include <cmath>
#include <cstring>
#include <thread>
#include <vector>

LidarSimAdapter::LidarSimAdapter(std::size_t point_count) : point_count_(point_count) {}

bool LidarSimAdapter::open() {
    return true;
}

bool LidarSimAdapter::start() {
    started_ = true;
    return true;
}

bool LidarSimAdapter::readFrame(SensorFrame& frame) {
    if (!started_) {
        return false;
    }

    std::vector<float> ranges(point_count_);
    for (std::size_t i = 0; i < point_count_; ++i) {
        ranges[i] = 10.0F + 2.0F * std::sin(static_cast<float>(i) * 0.01F +
                                            static_cast<float>(next_frame_id_) * 0.1F);
    }

    frame.frame_id = next_frame_id_++;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 1;
    frame.width = static_cast<std::uint32_t>(point_count_);
    frame.height = 1;
    frame.format = avm::kFormatLidarFloat32;
    frame.data_size = static_cast<std::uint32_t>(ranges.size() * sizeof(float));
    frame.data.resize(frame.data_size);
    std::memcpy(frame.data.data(), ranges.data(), frame.data_size);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
}

void LidarSimAdapter::stop() {
    started_ = false;
}