/**
 * @file gstreamer_camera_adapter.cpp
 * @brief GStreamer appsink camera adapter implementation with dependency-free NOT_COMPILED fallback.
 */
#include "gstreamer_camera_adapter.hpp"

#include "time_utils.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <mutex>
#include <vector>

#ifdef AVM_HAS_GSTREAMER
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#endif

namespace {
#ifdef AVM_HAS_GSTREAMER
void ensure_gst_initialized() {
    static std::once_flag once;
    std::call_once(once, []() {
        int argc = 0;
        char** argv = nullptr;
        gst_init(&argc, &argv);
    });
}

std::uint32_t caps_uint(GstCaps* caps, const char* key, std::uint32_t fallback) {
    if (!caps || gst_caps_is_empty(caps)) {
        return fallback;
    }
    GstStructure* st = gst_caps_get_structure(caps, 0);
    int v = 0;
    if (st && gst_structure_get_int(st, key, &v) && v > 0) {
        return static_cast<std::uint32_t>(v);
    }
    return fallback;
}

std::string caps_string(GstCaps* caps, const char* key, const std::string& fallback) {
    if (!caps || gst_caps_is_empty(caps)) {
        return fallback;
    }
    GstStructure* st = gst_caps_get_structure(caps, 0);
    const char* v = st ? gst_structure_get_string(st, key) : nullptr;
    return v ? std::string(v) : fallback;
}

std::string avm_format_from_caps(const std::string& gst_format, const std::string& fallback) {
    if (gst_format == "RGB" || gst_format == "BGR") {
        return "rgb";
    }
    if (gst_format == "NV12") {
        return "nv12";
    }
    if (gst_format == "YUY2" || gst_format == "YUYV") {
        return "yuyv";
    }
    return avm::gst_normalized_format(fallback);
}
#endif
}  // namespace

GStreamerCameraAdapter::GStreamerCameraAdapter(std::string device,
                                               std::uint32_t width,
                                               std::uint32_t height,
                                               std::uint32_t fps,
                                               std::string format,
                                               bool use_dmabuf,
                                               std::string custom_pipeline)
    : device_(std::move(device)),
      width_(width),
      height_(height),
      fps_(fps),
      format_(avm::gst_normalized_format(format)),
      use_dmabuf_(use_dmabuf),
      custom_pipeline_(std::move(custom_pipeline)) {
    if (!custom_pipeline_.empty()) {
        pipeline_ = custom_pipeline_;
    } else {
        avm::GStreamerCameraPipelineConfig cfg;
        cfg.device = device_;
        cfg.width = width_;
        cfg.height = height_;
        cfg.fps = fps_;
        cfg.format = format_;
        cfg.use_dmabuf = use_dmabuf_;
        pipeline_ = avm::build_gstreamer_v4l2_appsink_pipeline(cfg);
    }
}

GStreamerCameraAdapter::~GStreamerCameraAdapter() {
    stop();
}

bool GStreamerCameraAdapter::compiled() {
#ifdef AVM_HAS_GSTREAMER
    return true;
#else
    return false;
#endif
}

std::string GStreamerCameraAdapter::support_status() {
#ifdef AVM_HAS_GSTREAMER
    return "compiled: GStreamer appsink backend enabled";
#else
    return "not_compiled: rebuild with -DAVM_ENABLE_GSTREAMER=ON and install gstreamer-1.0/app/video development packages";
#endif
}

bool GStreamerCameraAdapter::open() {
#ifndef AVM_HAS_GSTREAMER
    std::cerr << "[GStreamerCameraAdapter] " << support_status() << "\n";
    return false;
#else
    ensure_gst_initialized();
    if (!avm::gstreamer_pipeline_has_appsink(pipeline_, "appsink")) {
        std::cerr << "[GStreamerCameraAdapter] pipeline must contain appsink name=appsink\n";
        return false;
    }

    GError* error = nullptr;
    pipeline_element_ = gst_parse_launch(pipeline_.c_str(), &error);
    if (!pipeline_element_) {
        std::cerr << "[GStreamerCameraAdapter] gst_parse_launch failed: "
                  << (error ? error->message : "unknown") << "\n";
        if (error) {
            g_error_free(error);
        }
        return false;
    }
    if (error) {
        g_error_free(error);
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_element_), "appsink");
    if (!appsink_) {
        std::cerr << "[GStreamerCameraAdapter] appsink name=appsink not found\n";
        gst_object_unref(pipeline_element_);
        pipeline_element_ = nullptr;
        return false;
    }

    g_object_set(G_OBJECT(appsink_), "emit-signals", FALSE, "sync", FALSE, "drop", TRUE, "max-buffers", 4, nullptr);
    opened_ = true;
    std::cout << "[GStreamerCameraAdapter] pipeline=" << pipeline_ << "\n";
    return true;
#endif
}

bool GStreamerCameraAdapter::start() {
#ifndef AVM_HAS_GSTREAMER
    return false;
#else
    if (!opened_ || !pipeline_element_) {
        return false;
    }
    const GstStateChangeReturn ret = gst_element_set_state(pipeline_element_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[GStreamerCameraAdapter] set PLAYING failed\n";
        return false;
    }
    started_ = true;
    return true;
#endif
}

bool GStreamerCameraAdapter::readFrame(SensorFrame& frame) {
#ifndef AVM_HAS_GSTREAMER
    (void)frame;
    return false;
#else
    if (!started_ || !appsink_) {
        return false;
    }

    GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink_), GST_SECOND * 2);
    if (!sample) {
        return false;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);
    GstMapInfo map{};
    if (!buffer || !gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return false;
    }

    const auto cap_width = caps_uint(caps, "width", width_);
    const auto cap_height = caps_uint(caps, "height", height_);
    const auto cap_format = caps_string(caps, "format", avm::gst_caps_format(format_));
    const auto normalized = avm_format_from_caps(cap_format, format_);

    frame = SensorFrame{};
    frame.frame_id = next_frame_id_++;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 0;
    frame.width = cap_width;
    frame.height = cap_height;
    frame.format = avm::gst_avm_format_code(normalized);
    frame.data_size = static_cast<std::uint32_t>(map.size);
    frame.stride_bytes = cap_height == 0 ? 0 : static_cast<std::uint32_t>(map.size / cap_height);
    frame.dmabuf_fd = -1;  // V2.6 maps/copies appsink memory; true DMA-BUF fd export is reserved for board-side V3.x.
    frame.data.assign(map.data, map.data + map.size);

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return !frame.data.empty();
#endif
}

void GStreamerCameraAdapter::stop() {
#ifdef AVM_HAS_GSTREAMER
    if (pipeline_element_) {
        gst_element_set_state(pipeline_element_, GST_STATE_NULL);
    }
    if (appsink_) {
        gst_object_unref(appsink_);
        appsink_ = nullptr;
    }
    if (pipeline_element_) {
        gst_object_unref(pipeline_element_);
        pipeline_element_ = nullptr;
    }
#endif
    opened_ = false;
    started_ = false;
}
