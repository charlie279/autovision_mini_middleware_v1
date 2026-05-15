/**
 * @file 11_camera_adapter_v4l2.cpp
 * @brief 示例：独立验证 V4L2 CameraAdapter，采集 YUYV 并保存第一帧 RGB888 PPM。
 */
#include "camera_adapter_v4l2.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
void save_ppm(const SensorFrame& frame, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[11_camera_adapter_v4l2] failed to open " << path << "\n";
        return;
    }
    out << "P6\n" << frame.width << " " << frame.height << "\n255\n";
    out.write(reinterpret_cast<const char*>(frame.data.data()), static_cast<std::streamsize>(frame.data.size()));
    std::cout << "[11_camera_adapter_v4l2] saved " << path << "\n";
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string device = (argc > 1) ? argv[1] : "/dev/video0";
    const auto width = static_cast<std::uint32_t>((argc > 2) ? std::stoul(argv[2]) : 640);
    const auto height = static_cast<std::uint32_t>((argc > 3) ? std::stoul(argv[3]) : 480);
    const auto fps = static_cast<std::uint32_t>((argc > 4) ? std::stoul(argv[4]) : 30);
    const int frame_count = (argc > 5) ? std::stoi(argv[5]) : 30;

    std::filesystem::create_directories("examples/logs");

    CameraAdapterV4L2 adapter(device, width, height, fps);
    if (!adapter.open() || !adapter.start()) {
        std::cerr << "[11_camera_adapter_v4l2] adapter open/start failed\n";
        return 1;
    }

    auto last = std::chrono::steady_clock::now();
    double sum_interval_ms = 0.0;
    int interval_count = 0;

    for (int i = 0; i < frame_count; ++i) {
        SensorFrame frame;
        if (!adapter.readFrame(frame)) {
            std::cerr << "[11_camera_adapter_v4l2] readFrame failed at i=" << i << "\n";
            adapter.stop();
            return 2;
        }

        const auto now = std::chrono::steady_clock::now();
        if (i > 0) {
            const double dt_ms = std::chrono::duration<double, std::milli>(now - last).count();
            sum_interval_ms += dt_ms;
            ++interval_count;
        }
        last = now;

        if (i == 0) {
            save_ppm(frame, "examples/logs/camera_first_frame.ppm");
        }

        if (i == 0 || i + 1 == frame_count || (i + 1) % 10 == 0) {
            std::cout << "[11_camera_adapter_v4l2] frame_id=" << frame.frame_id
                      << " sensor_type=" << frame.sensor_type
                      << " width=" << frame.width
                      << " height=" << frame.height
                      << " data_size=" << frame.data_size << "\n";
        }
    }

    adapter.stop();

    if (interval_count > 0) {
        std::cout << "[11_camera_adapter_v4l2] avg_interval_ms="
                  << (sum_interval_ms / static_cast<double>(interval_count)) << "\n";
    }
    std::cout << "[11_camera_adapter_v4l2] done frames=" << frame_count << "\n";
    return 0;
}