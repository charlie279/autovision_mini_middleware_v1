/**
 * @file 03_file_adapter.cpp
 * @brief 示例：直接调用 FileAdapter::open/start/readFrame/stop，理解文件输入到 SensorFrame 的流程。
 */
#include "avm_config.hpp"
#include "file_adapter.hpp"

#include <iostream>

int main() {
    FileAdapter adapter("assets/input_640x480_rgb.raw",
                        avm::kDefaultWidth,
                        avm::kDefaultHeight,
                        avm::kDefaultChannels);

    if (!adapter.open()) {
        std::cerr << "[03_file_adapter] open failed. Run: cd .. && ./scripts/prepare_input.sh\n";
        return 1;
    }
    if (!adapter.start()) {
        std::cerr << "[03_file_adapter] start failed\n";
        return 2;
    }

    SensorFrame frame;
    if (!adapter.readFrame(frame)) {
        std::cerr << "[03_file_adapter] readFrame failed\n";
        return 3;
    }

    std::cout << "[03_file_adapter] frame_id=" << frame.frame_id
              << " sensor_type=" << frame.sensor_type
              << " width=" << frame.width
              << " height=" << frame.height
              << " data_size=" << frame.data_size << "\n";

    adapter.stop();
    return 0;
}