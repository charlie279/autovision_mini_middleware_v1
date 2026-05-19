/**
 * @file gstreamer_camera_adapter.hpp
 * @brief V2.6 GStreamer camera adapter: v4l2src/caps/appsink -> AutoVision SensorFrame.
 */
#pragma once

#include "gstreamer_pipeline_config.hpp"
#include "sensor_adapter.hpp"

#include <cstdint>
#include <string>

#ifdef AVM_HAS_GSTREAMER
using GstElement = struct _GstElement;
using GstSample = struct _GstSample;
#endif

class GStreamerCameraAdapter final : public SensorAdapter {
public:
    GStreamerCameraAdapter(std::string device,
                           std::uint32_t width,
                           std::uint32_t height,
                           std::uint32_t fps,
                           std::string format,
                           bool use_dmabuf = false,
                           std::string custom_pipeline = "");
    ~GStreamerCameraAdapter() override;

    bool open() override;
    bool start() override;
    bool readFrame(SensorFrame& frame) override;
    void stop() override;

    const std::string& pipeline() const { return pipeline_; }
    static bool compiled();
    static std::string support_status();

private:
    std::string device_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t fps_ = 30;
    std::string format_ = "yuyv";
    bool use_dmabuf_ = false;
    std::string custom_pipeline_;
    std::string pipeline_;
    std::uint64_t next_frame_id_ = 1;
    bool opened_ = false;
    bool started_ = false;

#ifdef AVM_HAS_GSTREAMER
    GstElement* pipeline_element_ = nullptr;
    GstElement* appsink_ = nullptr;
#endif
};
