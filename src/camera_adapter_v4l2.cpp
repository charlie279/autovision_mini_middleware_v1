/**
 * @file camera_adapter_v4l2.cpp
 * @brief V1 摄像头适配器占位实现：提示用户 V2 再接真实 V4L2 ioctl + mmap。
 */
#include "camera_adapter_v4l2.hpp"

#include <iostream>

CameraAdapterV4L2::CameraAdapterV4L2(std::string device, std::uint32_t width, std::uint32_t height)
    : device_(std::move(device)), width_(width), height_(height) {}

bool CameraAdapterV4L2::open() {
    std::cerr << "[CameraAdapterV4L2] V1 stub only. Requested device=" << device_
              << " width=" << width_ << " height=" << height_ << "\n";
    std::cerr << "[CameraAdapterV4L2] Use --source file in V1. Implement ioctl+mmap in V2 board stage.\n";
    return false;
}

bool CameraAdapterV4L2::start() {
    return false;
}

bool CameraAdapterV4L2::readFrame(SensorFrame& frame) {
    (void)frame;
    return false;
}

void CameraAdapterV4L2::stop() {}
