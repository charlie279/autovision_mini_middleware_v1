/**
 * @file gstreamer_pipeline_config.cpp
 * @brief Dependency-free GStreamer pipeline string construction helpers.
 */
#include "gstreamer_pipeline_config.hpp"

#include "avm_config.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace avm {
namespace {
std::string lower_copy(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return v;
}

std::string shell_escape_path(const std::string& path) {
    // GStreamer parse-launch accepts a plain path for common /dev/videoX and logs/*.h264 cases.
    // Keep this helper intentionally conservative and dependency-free.
    return path;
}
}  // namespace

std::string gst_normalized_format(const std::string& format) {
    const auto f = lower_copy(format);
    if (f == "rgb" || f == "rgb888" || f == "rgb8") {
        return "rgb";
    }
    if (f == "nv12") {
        return "nv12";
    }
    return "yuyv";
}

std::string gst_caps_format(const std::string& format) {
    const auto f = gst_normalized_format(format);
    if (f == "rgb") {
        return "RGB";
    }
    if (f == "nv12") {
        return "NV12";
    }
    return "YUY2";  // GStreamer caps name for V4L2 YUYV/YUY2.
}

std::uint32_t gst_avm_format_code(const std::string& format) {
    const auto f = gst_normalized_format(format);
    if (f == "rgb") {
        return kFormatRgb888;
    }
    if (f == "nv12") {
        return kFormatNv12;
    }
    return kFormatYuyv;
}

std::size_t gst_bytes_per_pixel(const std::string& format) {
    const auto f = gst_normalized_format(format);
    if (f == "rgb") {
        return 3U;
    }
    // NV12 is 1.5 Bpp, so this helper returns the compact integer Bpp only for packed formats.
    // Use gst_frame_payload_size() when exact bytes are needed.
    if (f == "nv12") {
        return 1U;
    }
    return 2U;
}

std::size_t gst_frame_payload_size(std::uint32_t width, std::uint32_t height, const std::string& format) {
    const auto f = gst_normalized_format(format);
    const auto pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (f == "rgb") {
        return pixels * 3U;
    }
    if (f == "nv12") {
        return pixels * 3U / 2U;
    }
    return pixels * 2U;
}

std::string build_gstreamer_v4l2_appsink_pipeline(const GStreamerCameraPipelineConfig& cfg) {
    std::ostringstream oss;
    oss << "v4l2src device=" << shell_escape_path(cfg.device);
    if (cfg.use_dmabuf) {
        oss << " io-mode=dmabuf";
    }
    oss << " ! video/x-raw,format=" << gst_caps_format(cfg.format)
        << ",width=" << cfg.width
        << ",height=" << cfg.height
        << ",framerate=" << cfg.fps << "/1"
        << " ! queue max-size-buffers=4 leaky=downstream"
        << " ! appsink name=" << cfg.appsink_name << " sync=false drop=true max-buffers=4";
    return oss.str();
}

std::string build_gstreamer_appsrc_encode_pipeline(const GStreamerEncodePipelineConfig& cfg) {
    std::ostringstream oss;
    oss << "appsrc name=" << cfg.appsrc_name
        << " is-live=true format=time do-timestamp=true block=true"
        << " caps=video/x-raw,format=" << gst_caps_format(cfg.format)
        << ",width=" << cfg.width
        << ",height=" << cfg.height
        << ",framerate=" << cfg.fps << "/1";

    const auto codec = lower_copy(cfg.codec);
    if (codec == "raw") {
        oss << " ! filesink location=" << shell_escape_path(cfg.output) << " sync=false";
    } else if (codec == "h265" || codec == "hevc") {
        oss << " ! videoconvert"
            << " ! x265enc tune=zerolatency bitrate=" << cfg.bitrate_kbps
            << " key-int-max=" << cfg.fps
            << " ! h265parse"
            << " ! filesink location=" << shell_escape_path(cfg.output) << " sync=false";
    } else {
        oss << " ! videoconvert"
            << " ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=" << cfg.bitrate_kbps
            << " key-int-max=" << cfg.fps
            << " ! h264parse"
            << " ! filesink location=" << shell_escape_path(cfg.output) << " sync=false";
    }
    return oss.str();
}

bool gstreamer_pipeline_has_named_element(const std::string& pipeline, const std::string& name) {
    return pipeline.find("name=" + name) != std::string::npos;
}

bool gstreamer_pipeline_has_appsink(const std::string& pipeline, const std::string& appsink_name) {
    return pipeline.find("appsink") != std::string::npos && gstreamer_pipeline_has_named_element(pipeline, appsink_name);
}

bool gstreamer_pipeline_has_appsrc(const std::string& pipeline, const std::string& appsrc_name) {
    return pipeline.find("appsrc") != std::string::npos && gstreamer_pipeline_has_named_element(pipeline, appsrc_name);
}

}  // namespace avm
