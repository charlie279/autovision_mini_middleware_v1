#include "gstreamer_pipeline_config.hpp"

#include <iostream>
#include <string>

int main() {
    avm::GStreamerCameraPipelineConfig cam;
    cam.device = "/dev/video0";
    cam.width = 640;
    cam.height = 480;
    cam.fps = 30;
    cam.format = "yuyv";
    cam.use_dmabuf = true;
    const auto capture_pipeline = avm::build_gstreamer_v4l2_appsink_pipeline(cam);

    avm::GStreamerEncodePipelineConfig enc;
    enc.output = "logs/gstreamer_output.h264";
    enc.width = cam.width;
    enc.height = cam.height;
    enc.fps = cam.fps;
    enc.format = cam.format;
    enc.codec = "h264";
    const auto encode_pipeline = avm::build_gstreamer_appsrc_encode_pipeline(enc);

    const auto raw_payload = avm::gst_frame_payload_size(cam.width, cam.height, cam.format);
    const bool capture_ok = avm::gstreamer_pipeline_has_appsink(capture_pipeline) &&
                            capture_pipeline.find("v4l2src") != std::string::npos &&
                            capture_pipeline.find("format=YUY2") != std::string::npos &&
                            capture_pipeline.find("io-mode=dmabuf") != std::string::npos;
    const bool encode_ok = avm::gstreamer_pipeline_has_appsrc(encode_pipeline) &&
                           encode_pipeline.find("x264enc") != std::string::npos &&
                           encode_pipeline.find("filesink") != std::string::npos;
    const bool size_ok = raw_payload == 640U * 480U * 2U;
    const bool pass = capture_ok && encode_ok && size_ok;

    std::cout << "[26_gstreamer_pipeline_config] capture_pipeline=\"" << capture_pipeline << "\"\n";
    std::cout << "[26_gstreamer_pipeline_config] encode_pipeline=\"" << encode_pipeline << "\"\n";
    std::cout << "[26_gstreamer_pipeline_config] raw_payload_bytes=" << raw_payload
              << " capture_ok=" << capture_ok
              << " encode_ok=" << encode_ok
              << " size_ok=" << size_ok
              << " result=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
