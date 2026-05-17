
/**
 * @file 20_video_encode_benchmark.cpp
 * @brief V2.0 视频编码 backend benchmark。
 */
#include "avm_config.hpp"
#include "latency_profiler.hpp"
#include "sensor_frame.hpp"
#include "time_utils.hpp"
#include "video_encoder.hpp"
#include "video_encoder_factory.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
SensorFrame make_rgb_frame(std::uint32_t width, std::uint32_t height, std::uint64_t frame_id) {
    SensorFrame f;
    f.frame_id = frame_id;
    f.timestamp_ns = avm::now_ns();
    f.sensor_type = 2;
    f.width = width;
    f.height = height;
    f.format = avm::kFormatRgb888;
    f.stride_bytes = width * 3;
    f.data_size = width * height * 3;
    f.data.resize(f.data_size);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t off = (static_cast<std::size_t>(y) * width + x) * 3;
            f.data[off + 0] = static_cast<std::uint8_t>((x + frame_id * 3) & 0xFF);
            f.data[off + 1] = static_cast<std::uint8_t>((y + frame_id * 5) & 0xFF);
            f.data[off + 2] = static_cast<std::uint8_t>((x + y) & 0xFF);
        }
    }
    return f;
}

const char* codec_name(std::uint32_t codec) {
    return codec == kCodecH265 ? "H265" : "H264";
}

struct BenchResult {
    std::string backend;
    std::string codec;
    std::uint32_t frames = 0;
    double mean_ms = 0.0;
    double p95_ms = 0.0;
    double bitrate_kbps = 0.0;
};

BenchResult run_one(const std::string& backend, std::uint32_t codec,
                    std::uint32_t loops, std::uint32_t width, std::uint32_t height) {
    VideoEncodeConfig cfg;
    cfg.backend = backend;
    cfg.codec = codec;
    cfg.width = width;
    cfg.height = height;
    cfg.fps = 30;
    cfg.gop = 30;
    cfg.input_format = avm::kFormatRgb888;

    BenchResult r;
    r.backend = backend;
    r.codec = codec_name(codec);

    auto encoder = VideoEncoderFactory::create(cfg);
    if (!encoder->init(cfg)) {
        r.mean_ms = -1.0;
        r.p95_ms = -1.0;
        return r;
    }

    LatencyProfiler prof;
    for (std::uint32_t i = 0; i < loops; ++i) {
        auto frame = make_rgb_frame(width, height, i + 1);
        EncodedPacket pkt;
        if (encoder->encode(frame, pkt)) {
            prof.add(pkt.encode_ms);
            ++r.frames;
        }
    }
    std::vector<EncodedPacket> tail;
    encoder->flush(tail);
    r.frames += static_cast<std::uint32_t>(tail.size());
    r.mean_ms = prof.mean();
    r.p95_ms = prof.percentile(95.0);
    r.bitrate_kbps = encoder->avg_bitrate_kbps();
    r.backend = encoder->backend_name();
    encoder->release();
    return r;
}
}  // namespace

int main(int argc, char** argv) {
    std::uint32_t loops = 30;
    std::uint32_t width = avm::kDefaultWidth;
    std::uint32_t height = avm::kDefaultHeight;

    for (int i = 1; i + 1 < argc; ++i) {
        const std::string k = argv[i];
        if (k == "--loops") {
            loops = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (k == "--width") {
            width = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (k == "--height") {
            height = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        }
    }

    std::cout << "[20_video_encode_benchmark] loops=" << loops
              << " resolution=" << width << "x" << height << "\n";

    std::vector<BenchResult> results;
    results.push_back(run_one("soft", kCodecH264, loops, width, height));
    results.push_back(run_one("soft", kCodecH265, loops, width, height));
    results.push_back(run_one("mpp", kCodecH264, loops, width, height));
    results.push_back(run_one("v4l2m2m", kCodecH264, loops, width, height));

    for (const auto& r : results) {
        std::cout << "[20_video_encode_benchmark] backend=" << r.backend
                  << " codec=" << r.codec
                  << " frames=" << r.frames
                  << " mean_ms=" << r.mean_ms
                  << " p95_ms=" << r.p95_ms
                  << " bitrate_kbps=" << r.bitrate_kbps << "\n";
    }

    const std::string csv_path = "examples/logs/encode_benchmark.csv";
    std::ofstream csv(csv_path);
    if (csv.is_open()) {
        csv << "backend,codec,frames,mean_ms,p95_ms,bitrate_kbps\n";
        for (const auto& r : results) {
            csv << r.backend << "," << r.codec << "," << r.frames << ","
                << r.mean_ms << "," << r.p95_ms << "," << r.bitrate_kbps << "\n";
        }
    }
    std::cout << "[20_video_encode_benchmark] csv=" << csv_path << "\n";
    return 0;
}

