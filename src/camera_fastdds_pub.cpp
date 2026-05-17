/**
 * @file camera_fastdds_pub.cpp
 * @brief V2.4 two-process publisher: USB Camera/Synthetic raw frames -> FastDDS/RTPS.
 */
#include "avm_config.hpp"
#include "camera_adapter_v4l2.hpp"
#include "fastdds_rtps_transport.hpp"
#include "sensor_frame.hpp"
#include "time_utils.hpp"
#include "transport_message.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {
std::string arg_value(int argc, char** argv, const std::string& key, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return default_value;
}

bool has_flag(int argc, char** argv, const std::string& key) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == key) {
            return true;
        }
    }
    return false;
}

std::uint32_t bytes_per_pixel(const std::string& format) {
    if (format == "rgb" || format == "rgb888") {
        return 3;
    }
    return 2;
}

std::uint32_t avm_format_code(const std::string& format) {
    if (format == "rgb" || format == "rgb888") {
        return avm::kFormatRgb888;
    }
    return avm::kFormatYuyv;
}

CameraOutputFormat camera_output_format(const std::string& format) {
    if (format == "rgb" || format == "rgb888") {
        return CameraOutputFormat::RGB888;
    }
    return CameraOutputFormat::YUYV;
}

std::string normalized_format(const std::string& format) {
    if (format == "rgb" || format == "rgb888") {
        return "rgb";
    }
    return "yuyv";
}

SensorFrame make_synthetic_camera_frame(std::uint64_t seq,
                                        std::uint32_t width,
                                        std::uint32_t height,
                                        const std::string& format) {
    SensorFrame frame;
    frame.frame_id = seq;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 0;
    frame.width = width;
    frame.height = height;
    frame.format = avm_format_code(format);
    frame.stride_bytes = width * bytes_per_pixel(format);
    frame.data_size = frame.stride_bytes * height;
    frame.data.resize(frame.data_size);
    const std::uint8_t salt = (normalized_format(format) == "yuyv") ? 0x59U : 0x52U;
    for (std::size_t i = 0; i < frame.data.size(); ++i) {
        frame.data[i] = static_cast<std::uint8_t>((i * 131U + seq * 17U + salt) & 0xFFU);
    }
    return frame;
}

avm::TransportEnvelope to_transport_envelope(const SensorFrame& frame) {
    avm::TransportEnvelope msg;
    msg.kind = avm::TransportMessageKind::RAW_FRAME;
    msg.seq = frame.frame_id;
    msg.timestamp_us = frame.timestamp_ns / 1000U;
    msg.width = frame.width;
    msg.height = frame.height;
    msg.channels = (frame.height == 0 || frame.width == 0) ? 0U : (frame.data_size / (frame.width * frame.height));
    msg.raw_size = frame.data_size;
    msg.payload = frame.data;
    msg.payload_crc32 = avm::compute_transport_crc(msg.payload);
    return msg;
}

void write_pub_csv_header(std::ofstream& csv) {
    csv << "role,compiled,source,topic,format,width,height,frames_requested,warmup_frames,repeat,tx_attempts,sent,size_errors,payload_errors,status\n";
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string source = arg_value(argc, argv, "--source", "camera");
    const std::string device = arg_value(argc, argv, "--device", "/dev/video0");
    const std::string topic = arg_value(argc, argv, "--topic", "avm/camera/raw");
    const std::string format = normalized_format(arg_value(argc, argv, "--format", "yuyv"));
    const auto width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", "640")));
    const auto height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", "480")));
    const auto fps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--fps", "30")));
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "300")));
    const auto depth = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--depth", "512")));
    const auto domain_id = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--domain", "0")));
    const auto max_payload = static_cast<std::size_t>(std::stoull(arg_value(argc, argv, "--max-payload", "2097152")));
    const auto startup_ms = static_cast<int>(std::stoi(arg_value(argc, argv, "--startup-ms", "5000")));
    const auto linger_ms = static_cast<int>(std::stoi(arg_value(argc, argv, "--linger-ms", "5000")));
    const auto warmup_frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--warmup-frames", "10")));
    const auto repeat = std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--repeat", "5"))));
    const auto inter_repeat_us = static_cast<int>(std::stoi(arg_value(argc, argv, "--inter-repeat-us", "1000")));
    const std::string output = arg_value(argc, argv, "--output", "logs/benchmark_v2_4_fastdds_camera/camera_fastdds_pub.csv");
    const bool reliable = has_flag(argc, argv, "--reliable");
    const bool allow_unavailable = has_flag(argc, argv, "--allow-unavailable");

    std::filesystem::create_directories(std::filesystem::path(output).parent_path());
    std::ofstream csv(output);
    write_pub_csv_header(csv);

    if (!avm::fastdds_support_compiled()) {
        std::cout << "[camera_fastdds_pub] status=" << avm::fastdds_support_status() << "\n";
        csv << "pub,false," << source << "," << topic << "," << format << "," << width << "," << height
            << "," << frames << "," << warmup_frames << "," << repeat << ",0,0,0,0,NOT_COMPILED\n";
        return allow_unavailable ? 0 : 2;
    }

    avm::FastddsRtpsOptions options;
    options.topic = topic;
    options.domain_id = domain_id;
    options.depth = depth;
    options.max_payload_size = max_payload;
    options.reliable = reliable;
    options.participant_name = "autovision_camera_fastdds_pub";

    std::string error;
    avm::FastddsRtpsBus bus(options);
    if (!bus.open(&error)) {
        std::cerr << "[camera_fastdds_pub] open failed: " << error << "\n";
        csv << "pub,true," << source << "," << topic << "," << format << "," << width << "," << height
            << "," << frames << "," << warmup_frames << "," << repeat << ",0,0,0,0,OPEN_FAILED\n";
        return 3;
    }

    std::unique_ptr<SensorAdapter> adapter;
    if (source == "camera") {
        adapter = std::make_unique<CameraAdapterV4L2>(device, width, height, fps, camera_output_format(format));
        if (!adapter->open() || !adapter->start()) {
            std::cerr << "[camera_fastdds_pub] camera open/start failed device=" << device << "\n";
            csv << "pub,true," << source << "," << topic << "," << format << "," << width << "," << height
                << "," << frames << "," << warmup_frames << "," << repeat << ",0,0,0,0,CAMERA_OPEN_FAILED\n";
            return 4;
        }
    } else if (source != "synthetic") {
        std::cerr << "[camera_fastdds_pub] unsupported --source " << source << "\n";
        csv << "pub,true," << source << "," << topic << "," << format << "," << width << "," << height
            << "," << frames << "," << warmup_frames << "," << repeat << ",0,0,0,0,UNSUPPORTED_SOURCE\n";
        return 5;
    }

    if (startup_ms > 0) {
        std::cout << "[camera_fastdds_pub] startup_ms=" << startup_ms << " waiting before first publish\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(startup_ms));
    }

    std::uint64_t sent = 0;
    std::uint64_t tx_attempts = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    auto next_tick = std::chrono::steady_clock::now();
    const auto frame_period = std::chrono::microseconds(fps == 0 ? 0 : static_cast<int>(1000000U / fps));
    const auto repeat_gap = std::chrono::microseconds(std::max(0, inter_repeat_us));

    const std::uint64_t total_frames = frames + warmup_frames;
    for (std::uint64_t i = 0; i < total_frames; ++i) {
        SensorFrame frame;
        const std::uint64_t seq = i + 1U;
        if (source == "camera") {
            if (!adapter->readFrame(frame)) {
                std::cerr << "[camera_fastdds_pub] readFrame failed at seq=" << seq << "\n";
                break;
            }
            frame.frame_id = seq;
        } else {
            frame = make_synthetic_camera_frame(seq, width, height, format);
        }

        auto msg = to_transport_envelope(frame);
        const auto check = avm::validate_transport_message(msg, avm::TransportQosProfile{max_payload, depth, true, reliable});
        if (!check.ok) {
            size_errors += check.size_errors;
            payload_errors += check.payload_errors;
            std::cerr << "[camera_fastdds_pub] validation failed: " << check.reason << "\n";
            continue;
        }

        bool published_once = false;
        for (std::uint32_t r = 0; r < repeat; ++r) {
            if (!bus.publish(msg, &error)) {
                std::cerr << "[camera_fastdds_pub] publish failed seq=" << msg.seq << " repeat=" << r << ": " << error << "\n";
                continue;
            }
            ++tx_attempts;
            published_once = true;
            if (r + 1U < repeat && inter_repeat_us > 0) {
                std::this_thread::sleep_for(repeat_gap);
            }
        }
        if (published_once && i >= warmup_frames) {
            ++sent;
        }
        if (seq == 1 || seq == warmup_frames || sent == 1 || sent % 30 == 0) {
            std::cout << "[camera_fastdds_pub] seq=" << msg.seq
                      << " topic=" << topic
                      << " format=" << format
                      << " payload=" << msg.payload.size()
                      << " sent=" << sent
                      << " attempts=" << tx_attempts
                      << " warmup_frames=" << warmup_frames
                      << " repeat=" << repeat << "\n";
        }
        if (source == "synthetic" && fps > 0) {
            next_tick += frame_period;
            std::this_thread::sleep_until(next_tick);
        }
    }

    if (adapter) {
        adapter->stop();
    }
    if (linger_ms > 0) {
        std::cout << "[camera_fastdds_pub] linger_ms=" << linger_ms << " waiting before close\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(linger_ms));
    }

    const auto stats = bus.stats();
    const bool pass = (sent == frames && size_errors == 0 && payload_errors == 0 && stats.dropped == 0);
    csv << "pub,true," << source << "," << topic << "," << format << "," << width << "," << height
        << "," << frames << "," << warmup_frames << "," << repeat << "," << tx_attempts << "," << sent
        << "," << size_errors << "," << payload_errors << ","
        << (pass ? "PASS" : "PARTIAL") << "\n";
    std::cout << "[camera_fastdds_pub] done sent=" << sent
              << " tx_attempts=" << tx_attempts
              << " dropped=" << stats.dropped
              << " size_errors=" << size_errors
              << " payload_errors=" << payload_errors
              << " result=" << (pass ? "PASS" : "PARTIAL") << "\n";
    return pass ? 0 : 1;
}
