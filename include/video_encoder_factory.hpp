
/**
 * @file video_encoder_factory.hpp
 * @brief 视频编码器工厂，镜像 RuntimeFactory。
 */
#pragma once

#include "video_encoder.hpp"

#include <memory>
#include <string>

class VideoEncoderFactory {
public:
    static std::unique_ptr<VideoEncoder> create(const VideoEncodeConfig& cfg);
    static std::string available_backends();
};
