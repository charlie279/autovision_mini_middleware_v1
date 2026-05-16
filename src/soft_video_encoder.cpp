/**
 * @file soft_video_encoder.cpp
 * @brief Soft video encoder using ffmpeg CLI when available, with stub fallback.
 */
#include "soft_video_encoder.hpp"

#include "avm_config.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace {
void append_u32(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(static_cast<std::uint8_t>((x >> 24) & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<std::uint8_t>(x & 0xFF));
}

void append_u64(std::vector<std::uint8_t>& v, std::uint64_t x) {
    append_u32(v, static_cast<std::uint32_t>((x >> 32) & 0xFFFFFFFFULL));
    append_u32(v, static_cast<std::uint32_t>(x & 0xFFFFFFFFULL));
}
}  // namespace

SoftVideoEncoder::~SoftVideoEncoder() {
    release();
}

bool SoftVideoEncoder::ffmpeg_cli_available() {
    const int rc = std::system("command -v ffmpeg >/dev/null 2>&1");
    return rc == 0;
}

const char* SoftVideoEncoder::ffmpeg_pix_fmt(std::uint32_t fmt) {
    if (fmt == avm::kFormatYuyv) {
        return "yuyv422";
    }
    if (fmt == avm::kFormatNv12) {
        return "nv12";
    }
    return "rgb24";
}

bool SoftVideoEncoder::init(const VideoEncodeConfig& cfg) {
    cfg_ = cfg;
    cli_mode_ = !cfg_.output_path.empty() && ffmpeg_cli_available();
    if (cli_mode_) {
        raw_path_ = cfg_.output_path + ".raw_input";
        raw_out_.open(raw_path_, std::ios::binary);
        if (!raw_out_.is_open()) {
            std::cerr << "[SoftVideoEncoder] cannot open raw temp=" << raw_path_ << "\n";
            return false;
        }
    } else if (!cfg_.output_path.empty()) {
        out_.open(cfg_.output_path, std::ios::binary);
        if (!out_.is_open()) {
            std::cerr << "[SoftVideoEncoder] cannot open output=" << cfg_.output_path << "\n";
            return false;
        }
    }
    initialized_ = true;
    std::cout << "[SoftVideoEncoder] init ok mode=" << (cli_mode_ ? "ffmpeg_cli" : "soft_stub")
              << " codec=" << (cfg_.codec == kCodecH265 ? "H265" : "H264")
              << " " << cfg_.width << "x" << cfg_.height
              << " fps=" << cfg_.fps << " bitrate=" << cfg_.bitrate_kbps
              << "kbps gop=" << cfg_.gop << "\n";
    return true;
}

std::uint32_t SoftVideoEncoder::payload_checksum(const SensorFrame& frame) {
    std::uint32_t h = 2166136261u;
    const std::size_t step = std::max<std::size_t>(1, frame.data.size() / 4096);
    for (std::size_t i = 0; i < frame.data.size(); i += step) {
        h ^= frame.data[i];
        h *= 16777619u;
    }
    return h;
}

bool SoftVideoEncoder::encode(const SensorFrame& frame, EncodedPacket& pkt) {
    if (!initialized_) {
        return false;
    }
    const auto t0 = std::chrono::steady_clock::now();

    const bool keyframe = (frames_encoded_ == 0) || (cfg_.gop != 0 && (frames_encoded_ % cfg_.gop) == 0);
    const std::uint32_t checksum = payload_checksum(frame);

    if (cli_mode_ && raw_out_.is_open()) {
        raw_out_.write(reinterpret_cast<const char*>(frame.data.data()),
                       static_cast<std::streamsize>(frame.data.size()));
        cfg_.width = frame.width ? frame.width : cfg_.width;
        cfg_.height = frame.height ? frame.height : cfg_.height;
        cfg_.input_format = frame.format ? frame.format : cfg_.input_format;
    }

    pkt.data.clear();
    pkt.data.push_back(0x00);
    pkt.data.push_back(0x00);
    pkt.data.push_back(0x00);
    pkt.data.push_back(0x01);
    pkt.data.push_back(cfg_.codec == kCodecH265 ? 0x26 : 0x65);
    pkt.data.push_back(keyframe ? 0x01 : 0x00);
    append_u64(pkt.data, frame.frame_id);
    append_u64(pkt.data, static_cast<std::uint64_t>(frame.timestamp_ns));
    append_u32(pkt.data, frame.width);
    append_u32(pkt.data, frame.height);
    append_u32(pkt.data, frame.format);
    append_u32(pkt.data, checksum);
    const std::size_t sample = std::min<std::size_t>(frame.data.size(), keyframe ? 512 : 128);
    pkt.data.insert(pkt.data.end(), frame.data.begin(), frame.data.begin() + sample);

    pkt.pts_ns = static_cast<std::int64_t>(frame.timestamp_ns);
    pkt.frame_id = frame.frame_id;
    pkt.is_keyframe = keyframe;
    pkt.codec = cfg_.codec;

    const auto t1 = std::chrono::steady_clock::now();
    pkt.encode_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    profiler_.add(pkt.encode_ms);
    ++frames_encoded_;
    total_bytes_out_ += pkt.data.size();
    if (!cli_mode_) {
        write_packet(pkt);
    }
    return true;
}

bool SoftVideoEncoder::flush(std::vector<EncodedPacket>& pkts) {
    pkts.clear();
    if (cli_mode_ && !ffmpeg_done_) {
        if (raw_out_.is_open()) {
            raw_out_.close();
        }
        return run_ffmpeg_cli();
    }
    return true;
}

void SoftVideoEncoder::write_packet(const EncodedPacket& pkt) {
    if (out_.is_open() && !pkt.data.empty()) {
        out_.write(reinterpret_cast<const char*>(pkt.data.data()),
                   static_cast<std::streamsize>(pkt.data.size()));
    }
}

bool SoftVideoEncoder::run_ffmpeg_cli() {
    if (ffmpeg_done_ || raw_path_.empty() || cfg_.output_path.empty()) {
        return true;
    }
    std::filesystem::create_directories(std::filesystem::path(cfg_.output_path).parent_path());
    const char* encoder = (cfg_.codec == kCodecH265) ? "libx265" : "libx264";
    const char* muxer = (cfg_.codec == kCodecH265) ? "hevc" : "h264";
    std::ostringstream cmd;
    cmd << "ffmpeg -y -loglevel error "
        << "-f rawvideo -pix_fmt " << ffmpeg_pix_fmt(cfg_.input_format)
        << " -s " << cfg_.width << "x" << cfg_.height
        << " -r " << cfg_.fps
        << " -i '" << raw_path_ << "' "
        << "-an -c:v " << encoder
        << " -preset ultrafast -tune zerolatency -g " << cfg_.gop
        << " -b:v " << cfg_.bitrate_kbps << "k "
        << " -f " << muxer << " '" << cfg_.output_path << "'";
    const int rc = std::system(cmd.str().c_str());
    ffmpeg_done_ = (rc == 0);
    if (!ffmpeg_done_) {
        std::cerr << "[SoftVideoEncoder] ffmpeg CLI failed rc=" << rc << "\n";
        return false;
    }
    std::error_code ec;
    const auto bytes = std::filesystem::file_size(cfg_.output_path, ec);
    if (!ec && bytes > 0) {
        total_bytes_out_ = static_cast<std::uint64_t>(bytes);
    }
    std::filesystem::remove(raw_path_, ec);
    return true;
}

void SoftVideoEncoder::release() {
    if (cli_mode_ && !ffmpeg_done_) {
        if (raw_out_.is_open()) {
            raw_out_.close();
        }
        run_ffmpeg_cli();
    }
    if (out_.is_open()) {
        out_.close();
    }
    initialized_ = false;
}

double SoftVideoEncoder::avg_bitrate_kbps() const {
    if (frames_encoded_ == 0 || cfg_.fps == 0) {
        return 0.0;
    }
    const double seconds = static_cast<double>(frames_encoded_) / static_cast<double>(cfg_.fps);
    return (static_cast<double>(total_bytes_out_) * 8.0 / 1000.0) / seconds;
}
