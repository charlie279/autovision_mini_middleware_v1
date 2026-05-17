
/**
 * @file mpp_video_encoder.cpp
 * @brief Rockchip MPP encoder placeholder/stub.
 */
#include "mpp_video_encoder.hpp"

#include <chrono>
#include <iostream>

MppVideoEncoder::~MppVideoEncoder() {
    release();
}

bool MppVideoEncoder::is_real_mpp_available() {
#ifdef AVM_HAS_MPP
    return true;
#else
    return false;
#endif
}

bool MppVideoEncoder::init(const VideoEncodeConfig& cfg) {
    cfg_ = cfg;
    if (!cfg_.output_path.empty()) {
        out_.open(cfg_.output_path, std::ios::binary);
        if (!out_.is_open()) {
            std::cerr << "[MppVideoEncoder] cannot open output=" << cfg_.output_path << "\n";
            return false;
        }
    }
    initialized_ = true;
    std::cout << "[MppVideoEncoder] init "
              << (is_real_mpp_available() ? "real_or_board_build" : "stub")
              << " dmabuf_input=" << (cfg_.dmabuf_input ? "true" : "false")
              << " codec=" << (cfg_.codec == kCodecH265 ? "H265" : "H264") << "\n";
    return true;
}

bool MppVideoEncoder::encode(const SensorFrame& frame, EncodedPacket& pkt) {
    if (!initialized_) {
        return false;
    }
    const auto t0 = std::chrono::steady_clock::now();
    pkt.data = {0x00, 0x00, 0x00, 0x01, 0x4D, 0x50, 0x50}; // "MPP" marker
    pkt.data.push_back(static_cast<std::uint8_t>(frame.dmabuf_fd >= 0 ? 1 : 0));
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
    write_packet(pkt);
    return true;
}

bool MppVideoEncoder::flush(std::vector<EncodedPacket>& pkts) {
    pkts.clear();
    return true;
}

void MppVideoEncoder::write_packet(const EncodedPacket& pkt) {
    if (out_.is_open() && !pkt.data.empty()) {
        out_.write(reinterpret_cast<const char*>(pkt.data.data()),
                   static_cast<std::streamsize>(pkt.data.size()));
    }
}

void MppVideoEncoder::release() {
    if (out_.is_open()) {
        out_.close();
    }
    initialized_ = false;
}

double MppVideoEncoder::avg_bitrate_kbps() const {
    if (frames_encoded_ == 0 || cfg_.fps == 0) {
        return 0.0;
    }
    const double seconds = static_cast<double>(frames_encoded_) / static_cast<double>(cfg_.fps);
    return (static_cast<double>(total_bytes_out_) * 8.0 / 1000.0) / seconds;
}

