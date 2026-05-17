
/**
 * @file video_encoder_factory.cpp
 * @brief 视频编码器工厂实现。
 */
#include "video_encoder_factory.hpp"

#include "mpp_video_encoder.hpp"
#include "soft_video_encoder.hpp"
#include "v4l2_m2m_encoder.hpp"

#include <stdexcept>

std::unique_ptr<VideoEncoder> VideoEncoderFactory::create(const VideoEncodeConfig& cfg) {
    if (cfg.backend == "soft") {
        return std::make_unique<SoftVideoEncoder>();
    }
    if (cfg.backend == "v4l2m2m") {
        return std::make_unique<V4l2M2mEncoder>();
    }
    if (cfg.backend == "mpp") {
        return std::make_unique<MppVideoEncoder>();
    }
    throw std::invalid_argument("[VideoEncoderFactory] unknown backend: " + cfg.backend);
}

std::string VideoEncoderFactory::available_backends() {
    return "soft v4l2m2m mpp";
}

