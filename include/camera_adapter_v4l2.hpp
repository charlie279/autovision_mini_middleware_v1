/**
 * @file camera_adapter_v4l2.hpp
 * @brief V4L2 摄像头适配器占位文件：V1 仅保留接口，V2 在开发板上实现 ioctl + mmap。
 */
#pragma once

#include "sensor_adapter.hpp"

#include <cstdint>
#include <string>

class CameraAdapterV4L2 final : public SensorAdapter {
public:
    CameraAdapterV4L2(std::string device, std::uint32_t width, std::uint32_t height);

    bool open() override;
    bool start() override;
    bool readFrame(SensorFrame& frame) override;
    void stop() override;

private:
    std::string device_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};
