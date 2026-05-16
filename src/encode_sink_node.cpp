
/**
 * @file encode_sink_node.cpp
 * @brief 独立编码进程：消费 FrameMeta Ring + ShmFramePool，调用 VideoEncoder 写出码流。
 */
#include "avm_config.hpp"
#include "frame_meta.hpp"
#include "latency_profiler.hpp"
#include "sensor_frame.hpp"
#include "shm_frame_pool.hpp"
#include "shm_ring_buffer.hpp"
#include "time_utils.hpp"
#include "video_encoder.hpp"
#include "video_encoder_factory.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
volatile std::sig_atomic_t g_running = 1;

void handle_signal(int) {
    g_running = 0;
}

std::string arg_value(int argc, char** argv, const std::string& key, const std::string& def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return def;
}

bool has_flag(int argc, char** argv, const std::string& key) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == key) {
            return true;
        }
    }
    return false;
}

std::uint32_t parse_codec(const std::string& s) {
    return (s == "h265" || s == "H265" || s == "hevc" || s == "HEVC") ? kCodecH265 : kCodecH264;
}

const char* codec_name(std::uint32_t codec) {
    return codec == kCodecH265 ? "H265" : "H264";
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    if (has_flag(argc, argv, "--list-backends")) {
        std::cout << "Available backends: " << VideoEncoderFactory::available_backends() << "\n";
        return 0;
    }

    VideoEncodeConfig cfg;
    cfg.backend = arg_value(argc, argv, "--backend", "soft");
    cfg.codec = parse_codec(arg_value(argc, argv, "--codec", "h264"));
    cfg.width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", std::to_string(avm::kDefaultWidth))));
    cfg.height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", std::to_string(avm::kDefaultHeight))));
    cfg.fps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--fps", std::to_string(avm::kDefaultFps))));
    cfg.bitrate_kbps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--bitrate", "2000")));
    cfg.gop = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--gop", "30")));
    cfg.output_path = arg_value(argc, argv, "--output", cfg.codec == kCodecH265 ? "logs/encode_output.h265" : "logs/encode_output.h264");
    cfg.dmabuf_input = has_flag(argc, argv, "--dmabuf");
    cfg.input_format = avm::kFormatRgb888;

    const auto max_frames = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--frames", "120")));
    const bool do_benchmark = has_flag(argc, argv, "--benchmark");
    const std::string csv_out = arg_value(argc, argv, "--csv", "logs/encode_benchmark.csv");

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    ShmFramePool frame_pool;
    if (!frame_pool.open(avm::kFramePoolName, avm::kFramePoolCount, avm::kFrameBufferSize)) {
        std::cerr << "[encode_sink_node] cannot open FramePool. Start media_node first.\n";
        return 1;
    }

    ShmRingBuffer frame_ring;
    if (!frame_ring.open(avm::kFrameMetaRingName, sizeof(FrameMeta), avm::kRingCapacity)) {
        std::cerr << "[encode_sink_node] cannot open FrameMetaRing. Start media_node first.\n";
        return 2;
    }

    auto encoder = VideoEncoderFactory::create(cfg);
    if (!encoder->init(cfg)) {
        std::cerr << "[encode_sink_node] encoder init failed backend=" << cfg.backend << "\n";
        return 3;
    }

    std::cout << "[encode_sink_node] started backend=" << encoder->backend_name()
              << " codec=" << codec_name(cfg.codec)
              << " frames=" << max_frames
              << " output=" << cfg.output_path << "\n";

    LatencyProfiler loop_profiler;
    std::uint32_t frames_consumed = 0;
    std::uint32_t encode_ok = 0;
    std::uint32_t empty_polls = 0;

    while (g_running && frames_consumed < max_frames) {
        FrameMeta meta{};
        if (!frame_ring.pop(&meta, sizeof(meta), 1000)) {
            ++empty_polls;
            continue;
        }

        SensorFrame frame;
        frame.frame_id = meta.frame_id;
        frame.timestamp_ns = meta.timestamp_ns;
        frame.sensor_type = meta.sensor_type;
        frame.width = meta.width ? meta.width : cfg.width;
        frame.height = meta.height ? meta.height : cfg.height;
        frame.format = meta.format ? meta.format : avm::kFormatRgb888;
        frame.stride_bytes = meta.stride_bytes;
        frame.data_size = meta.data_size;
        frame.dmabuf_fd = -1;

        const std::size_t payload_size = std::min<std::size_t>(
            meta.data_size ? meta.data_size : static_cast<std::size_t>(frame.width) * frame.height * 3,
            avm::kFrameBufferSize);
        frame.data.resize(payload_size);
        if (!frame_pool.read(meta.buffer_index, frame.data.data(), payload_size)) {
            std::cerr << "[encode_sink_node] frame_pool.read failed frame_id=" << meta.frame_id << "\n";
            continue;
        }
        frame.data_size = static_cast<std::uint32_t>(payload_size);

        cfg.width = frame.width;
        cfg.height = frame.height;
        cfg.input_format = frame.format;

        const auto t0 = std::chrono::steady_clock::now();
        EncodedPacket pkt;
        if (encoder->encode(frame, pkt)) {
            ++encode_ok;
            const auto t1 = std::chrono::steady_clock::now();
            loop_profiler.add(std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        ++frames_consumed;

        if (frames_consumed == 1 || frames_consumed % 30 == 0) {
            std::cout << "[encode_sink_node] frame_id=" << meta.frame_id
                      << " encoded=" << encode_ok
                      << " packet_bytes=" << pkt.data.size()
                      << " encode_ms=" << pkt.encode_ms << "\n";
        }
    }

    std::vector<EncodedPacket> tail;
    encoder->flush(tail);
    encode_ok += static_cast<std::uint32_t>(tail.size());

    std::cout << "[encode_sink_node] done frames_consumed=" << frames_consumed
              << " encode_ok=" << encode_ok
              << " empty_polls=" << empty_polls
              << " encode_mean_ms=" << encoder->avg_encode_ms()
              << " actual_bitrate_kbps=" << encoder->avg_bitrate_kbps() << "\n";

    if (do_benchmark) {
        std::ofstream csv(csv_out);
        if (csv.is_open()) {
            csv << "backend,codec,width,height,fps,bitrate_kbps_target,frames_encoded,encode_mean_ms,loop_p95_ms,actual_bitrate_kbps\n";
            csv << encoder->backend_name() << "," << codec_name(cfg.codec) << ","
                << cfg.width << "," << cfg.height << "," << cfg.fps << ","
                << cfg.bitrate_kbps << "," << encode_ok << ","
                << encoder->avg_encode_ms() << "," << loop_profiler.percentile(95.0) << ","
                << encoder->avg_bitrate_kbps() << "\n";
            std::cout << "[encode_sink_node] csv=" << csv_out << "\n";
        }
    }

    encoder->release();
    return encode_ok == 0 ? 4 : 0;
}
