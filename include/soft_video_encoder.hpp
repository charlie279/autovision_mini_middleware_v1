/**
 * @file soft_video_encoder.hpp
 * @brief VMware/Linux 侧软件编码 backend。
 *
 * 默认实现不依赖 FFmpeg 开发头文件：当系统存在 ffmpeg CLI 且 output_path 非空时，
 * encode() 将原始帧写入临时 raw 文件，release()/flush 后调用 ffmpeg 生成真实
 * H.264/H.265 Annex-B 裸流；当 output_path 为空或无 ffmpeg CLI 时，退回
 * Annex-B-like soft_stub，用于 benchmark/CI 流程验证。
 */
#pragma once

#include "latency_profiler.hpp"
#include "video_encoder.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class SoftVideoEncoder final : public VideoEncoder {
public:
    SoftVideoEncoder() = default;
    ~SoftVideoEncoder() override;

    bool init(const VideoEncodeConfig& cfg) override;
    bool encode(const SensorFrame& frame, EncodedPacket& pkt) override;
    bool flush(std::vector<EncodedPacket>& pkts) override;
    void release() override;

    std::string backend_name() const override { return cli_mode_ ? "soft_ffmpeg_cli" : "soft_stub"; }
    std::uint32_t frames_encoded() const override { return frames_encoded_; }
    double avg_encode_ms() const override { return profiler_.mean(); }
    double avg_bitrate_kbps() const override;

private:
    void write_packet(const EncodedPacket& pkt);
    bool run_ffmpeg_cli();
    static bool ffmpeg_cli_available();
    static const char* ffmpeg_pix_fmt(std::uint32_t fmt);
    static std::uint32_t payload_checksum(const SensorFrame& frame);

    VideoEncodeConfig cfg_{};
    std::ofstream out_;
    std::ofstream raw_out_;
    std::string raw_path_;
    LatencyProfiler profiler_;
    std::uint32_t frames_encoded_ = 0;
    std::uint64_t total_bytes_out_ = 0;
    bool initialized_ = false;
    bool cli_mode_ = false;
    bool ffmpeg_done_ = false;
};
