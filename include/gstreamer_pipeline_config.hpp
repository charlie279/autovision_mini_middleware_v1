/**
 * @file gstreamer_pipeline_config.hpp
 * @brief Dependency-free helpers for V2.6 GStreamer camera capture / encode pipeline strings.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace avm {

struct GStreamerCameraPipelineConfig {
    std::string device = "/dev/video0";
    std::uint32_t width = 640;
    std::uint32_t height = 480;
    std::uint32_t fps = 30;
    std::string format = "yuyv";       // yuyv/rgb/nv12
    bool use_dmabuf = false;           // requests v4l2src io-mode=dmabuf, still mapped/copy-read by appsink in V2.6
    std::string appsink_name = "appsink";
};

struct GStreamerEncodePipelineConfig {
    std::string output = "logs/gstreamer_output.h264";
    std::uint32_t width = 640;
    std::uint32_t height = 480;
    std::uint32_t fps = 30;
    std::string format = "yuyv";       // appsrc input format
    std::string codec = "h264";        // h264/h265/raw
    std::uint32_t bitrate_kbps = 4000;
    std::string appsrc_name = "appsrc";
};

std::string gst_normalized_format(const std::string& format);
std::string gst_caps_format(const std::string& format);
std::uint32_t gst_avm_format_code(const std::string& format);
std::size_t gst_bytes_per_pixel(const std::string& format);
std::size_t gst_frame_payload_size(std::uint32_t width, std::uint32_t height, const std::string& format);
std::string build_gstreamer_v4l2_appsink_pipeline(const GStreamerCameraPipelineConfig& cfg);
std::string build_gstreamer_appsrc_encode_pipeline(const GStreamerEncodePipelineConfig& cfg);
bool gstreamer_pipeline_has_named_element(const std::string& pipeline, const std::string& name);
bool gstreamer_pipeline_has_appsink(const std::string& pipeline, const std::string& appsink_name = "appsink");
bool gstreamer_pipeline_has_appsrc(const std::string& pipeline, const std::string& appsrc_name = "appsrc");

}  // namespace avm
