
/**
 * @file video_encoder.hpp
 * @brief 视频编码器抽象接口，镜像 InferenceRuntime 设计。
 *
 * V2.0 目标：把视频编码能力从 media_node 中解耦为独立可替换 backend。
 * VMware/Linux 默认可使用 dependency-free soft_stub 进行编译和流程验证；
 * 安装 FFmpeg 开发包后，可在 SoftVideoEncoder 内继续替换为真实 libx264/libx265 路径；
 * RK3588/OPi5+ 侧可将 mpp backend 从 stub 替换为真实 Rockchip MPP。
 */
#pragma once

#include "sensor_frame.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

constexpr std::uint32_t kCodecH264 = 0x48323634U;  // 'H264'
constexpr std::uint32_t kCodecH265 = 0x48323635U;  // 'H265'

struct VideoEncodeConfig {
    std::string backend = "soft";        // soft | v4l2m2m | mpp
    std::uint32_t codec = kCodecH264;
    std::uint32_t width = 640;
    std::uint32_t height = 480;
    std::uint32_t fps = 30;
    std::uint32_t bitrate_kbps = 2000;
    std::uint32_t gop = 30;
    std::uint32_t input_format = 0;
    std::string output_path;
    bool dmabuf_input = false;
};

struct EncodedPacket {
    std::vector<std::uint8_t> data;
    std::int64_t pts_ns = 0;
    std::uint64_t frame_id = 0;
    bool is_keyframe = false;
    std::uint32_t codec = kCodecH264;
    double encode_ms = 0.0;
};

class VideoEncoder {
public:
    virtual ~VideoEncoder() = default;

    virtual bool init(const VideoEncodeConfig& cfg) = 0;
    virtual bool encode(const SensorFrame& frame, EncodedPacket& pkt) = 0;
    virtual bool flush(std::vector<EncodedPacket>& pkts) = 0;
    virtual void release() = 0;

    virtual std::string backend_name() const = 0;
    virtual std::uint32_t frames_encoded() const = 0;
    virtual double avg_encode_ms() const = 0;
    virtual double avg_bitrate_kbps() const = 0;
};

