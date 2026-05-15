/**
 * @file sensor_adapter.hpp
 * @brief 传感器抽象接口：屏蔽文件输入、模拟雷达和未来真实摄像头的差异。
 */
#pragma once

#include "sensor_frame.hpp"

class SensorAdapter {
public:
    virtual bool open() = 0;
    virtual bool start() = 0;
    virtual bool readFrame(SensorFrame& frame) = 0;
    virtual void stop() = 0;
    virtual ~SensorAdapter() = default;
};