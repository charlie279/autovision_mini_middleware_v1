/**
 * @file 04_lidar_sim_adapter.cpp
 * @brief 示例：直接调用 LidarSimAdapter，理解模拟 Lidar 输入如何复用 SensorAdapter 接口。
 */
#include "lidar_sim_adapter.hpp"

#include <cstring>
#include <iostream>

int main() {
    LidarSimAdapter adapter(16);
    adapter.open();
    adapter.start();

    SensorFrame frame;
    if (!adapter.readFrame(frame)) {
        std::cerr << "[04_lidar_sim_adapter] readFrame failed\n";
        return 1;
    }

    float first_range = 0.0F;
    if (frame.data_size >= sizeof(float)) {
        std::memcpy(&first_range, frame.data.data(), sizeof(float));
    }

    std::cout << "[04_lidar_sim_adapter] frame_id=" << frame.frame_id
              << " sensor_type=" << frame.sensor_type
              << " points=" << frame.width
              << " data_size=" << frame.data_size
              << " first_range=" << first_range << "\n";
    adapter.stop();
    return 0;
}