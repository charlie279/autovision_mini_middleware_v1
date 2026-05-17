
/**
 * @file mpp_video_encoder.hpp
 * @brief Rockchip MPP backend 接口。
 *
 * VMware 默认 stub 编译；RK3588/OPi5+ 安装 librockchip-mpp-dev 后，
 * 可在 src/mpp_video_encoder.cpp 内替换为真实 mpp_create/mpp_init/
 * mpp_buffer_import_with_tag/encode_put_frame/encode_get_packet 路径。
 */
#pragma once

#include "latency_profiler.hpp"
#include "video_encoder.hpp"

#include <cstdint>
#include <fstream>

class MppVideoEncoder final : public VideoEncoder {
public:
    MppVideoEncoder() = default;
    ~MppVideoEncoder() override;

    bool init(const VideoEncodeConfig& cfg) override;
    bool encode(const SensorFrame& frame, EncodedPacket& pkt) override;
    bool flush(std::vector<EncodedPacket>& pkts) override;
    void release() override;

    std::string backend_name() const override { return is_real_mpp_available() ? "mpp" : "mpp_stub"; }
    std::uint32_t frames_encoded() const override { return frames_encoded_; }
    double avg_encode_ms() const override { return profiler_.mean(); }
    double avg_bitrate_kbps() const override;

    static bool is_real_mpp_available();

private:
    void write_packet(const EncodedPacket& pkt);

    VideoEncodeConfig cfg_{};
    std::ofstream out_;
    LatencyProfiler profiler_;
    std::uint32_t frames_encoded_ = 0;
    std::uint64_t total_bytes_out_ = 0;
    bool initialized_ = false;
};

