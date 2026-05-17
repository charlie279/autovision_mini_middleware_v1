
/**
 * @file v4l2_m2m_encoder.hpp
 * @brief V4L2 M2M backend 接口。
 *
 * 本文件保留 Linux 标准硬件编码接口边界。当前实现为可编译 stub，
 * 用于在没有 /dev/video M2M 设备的 VMware/CI 环境下保持工程可运行。
 * 板端接入时可在 src/v4l2_m2m_encoder.cpp 内替换为 VIDIOC_REQBUFS/
 * OUTPUT queue/CAPTURE queue 的真实实现。
 */
#pragma once

#include "latency_profiler.hpp"
#include "video_encoder.hpp"

#include <cstdint>
#include <string>
#include <vector>

class V4l2M2mEncoder final : public VideoEncoder {
public:
    explicit V4l2M2mEncoder(std::string device = "/dev/video11");
    ~V4l2M2mEncoder() override;

    bool init(const VideoEncodeConfig& cfg) override;
    bool encode(const SensorFrame& frame, EncodedPacket& pkt) override;
    bool flush(std::vector<EncodedPacket>& pkts) override;
    void release() override;

    std::string backend_name() const override { return "v4l2m2m"; }
    std::uint32_t frames_encoded() const override { return frames_encoded_; }
    double avg_encode_ms() const override { return profiler_.mean(); }
    double avg_bitrate_kbps() const override;

    static std::vector<std::string> enumerate_m2m_devices();

private:
    std::string device_;
    VideoEncodeConfig cfg_{};
    LatencyProfiler profiler_;
    std::uint32_t frames_encoded_ = 0;
    std::uint64_t total_bytes_out_ = 0;
    bool initialized_ = false;
};

