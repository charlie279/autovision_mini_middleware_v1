/**
 * @file lidar_sim_adapter.hpp
 * @brief 模拟 Lidar 适配器：周期性生成 float32 距离数组，用于证明传感器抽象能力。
 */
#pragma once

#include "sensor_adapter.hpp"

#include <cstddef>
#include <cstdint>

class LidarSimAdapter final : public SensorAdapter {
public:
    explicit LidarSimAdapter(std::size_t point_count = 1024);

    bool open() override;
    bool start() override;
    bool readFrame(SensorFrame& frame) override;
    void stop() override;

private:
    std::size_t point_count_ = 1024;
    std::uint64_t next_frame_id_ = 1;
    bool started_ = false;
};
