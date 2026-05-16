
/**
 * @file v4l2_m2m_encoder.cpp
 * @brief V4L2 M2M hardware encoder placeholder.
 */
#include "v4l2_m2m_encoder.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>

V4l2M2mEncoder::V4l2M2mEncoder(std::string device) : device_(std::move(device)) {}

V4l2M2mEncoder::~V4l2M2mEncoder() {
    release();
}

bool V4l2M2mEncoder::init(const VideoEncodeConfig& cfg) {
    cfg_ = cfg;
    initialized_ = true;
    std::cout << "[V4l2M2mEncoder] init stub device=" << device_
              << " codec=" << (cfg_.codec == kCodecH265 ? "H265" : "H264")
              << " note=real V4L2 M2M path requires board codec device\n";
    return true;
}

bool V4l2M2mEncoder::encode(const SensorFrame& frame, EncodedPacket& pkt) {
    if (!initialized_) {
        return false;
    }
    const auto t0 = std::chrono::steady_clock::now();
    pkt.data = {0x00, 0x00, 0x00, 0x01, 0x7E, 0x56, 0x34, 0x4C}; // stub marker
    pkt.data.push_back(static_cast<std::uint8_t>(frame.frame_id & 0xFF));
    pkt.pts_ns = static_cast<std::int64_t>(frame.timestamp_ns);
    pkt.frame_id = frame.frame_id;
    pkt.is_keyframe = frames_encoded_ == 0 || (cfg_.gop && frames_encoded_ % cfg_.gop == 0);
    pkt.codec = cfg_.codec;
    const auto t1 = std::chrono::steady_clock::now();
    pkt.encode_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    profiler_.add(pkt.encode_ms);
    ++frames_encoded_;
    total_bytes_out_ += pkt.data.size();
    return true;
}

bool V4l2M2mEncoder::flush(std::vector<EncodedPacket>& pkts) {
    pkts.clear();
    return true;
}

void V4l2M2mEncoder::release() {
    initialized_ = false;
}

double V4l2M2mEncoder::avg_bitrate_kbps() const {
    if (frames_encoded_ == 0 || cfg_.fps == 0) {
        return 0.0;
    }
    const double seconds = static_cast<double>(frames_encoded_) / static_cast<double>(cfg_.fps);
    return (static_cast<double>(total_bytes_out_) * 8.0 / 1000.0) / seconds;
}

std::vector<std::string> V4l2M2mEncoder::enumerate_m2m_devices() {
    std::vector<std::string> devices;
    for (int i = 0; i < 64; ++i) {
        std::string p = "/dev/video" + std::to_string(i);
        if (std::filesystem::exists(p)) {
            devices.push_back(p);
        }
    }
    return devices;
}
