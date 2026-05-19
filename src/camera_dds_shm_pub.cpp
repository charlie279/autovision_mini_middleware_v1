/**
 * @file camera_dds_shm_pub.cpp
 * @brief V2.5 publisher: Camera/Synthetic raw payload -> POSIX SHM, descriptor metadata -> FastDDS/RTPS.
 */
#include "avm_config.hpp"
#include "camera_adapter_v4l2.hpp"
#include "dds_shm_frame_meta.hpp"
#include "fastdds_rtps_transport.hpp"
#include "sensor_frame.hpp"
#include "shm_frame_pool.hpp"
#include "time_utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

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

void ensure_parent_dir(const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

CameraOutputFormat camera_output_format(const std::string& format) {
    const auto f = avm::dds_shm_normalized_format(format);
    if (f == "rgb") {
        return CameraOutputFormat::RGB888;
    }
    return CameraOutputFormat::YUYV;
}

SensorFrame make_synthetic_camera_frame(std::uint64_t seq,
                                        std::uint32_t width,
                                        std::uint32_t height,
                                        const std::string& format) {
    const auto normalized = avm::dds_shm_normalized_format(format);
    const auto bpp = avm::dds_shm_bytes_per_pixel(normalized);
    SensorFrame frame;
    frame.frame_id = seq;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 0;
    frame.width = width;
    frame.height = height;
    frame.format = avm::dds_shm_format_code(normalized);
    frame.stride_bytes = width * bpp;
    frame.data_size = frame.stride_bytes * height;
    frame.data.resize(frame.data_size);
    const std::uint8_t salt = (normalized == "rgb") ? 0x52U : ((normalized == "nv12") ? 0x4EU : 0x59U);
    for (std::size_t i = 0; i < frame.data.size(); ++i) {
        frame.data[i] = static_cast<std::uint8_t>((i * 131U + seq * 17U + salt) & 0xFFU);
    }
    return frame;
}

void write_pub_csv_header(std::ofstream& csv) {
    csv << "role,compiled,source,topic,shm_name,format,width,height,frames_requested,warmup_frames,repeat,pool_count,buffer_size,metadata_bytes,raw_payload_bytes,tx_attempts,sent,shm_write_errors,metadata_errors,payload_errors,status\n";
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string source = arg_value(argc, argv, "--source", "camera");
    const std::string device = arg_value(argc, argv, "--device", "/dev/video0");
    const std::string topic = arg_value(argc, argv, "--topic", "avm/camera/dds_shm_meta");
    const std::string shm_name = arg_value(argc, argv, "--shm-name", "/avm_camera_dds_shm_pool");
    const std::string format = avm::dds_shm_normalized_format(arg_value(argc, argv, "--format", "yuyv"));
    const auto width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", "640")));
    const auto height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", "480")));
    const auto fps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--fps", "30")));
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "300")));
    const auto depth = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--depth", "1024")));
    const auto domain_id = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--domain", "0")));
    const auto max_payload = static_cast<std::size_t>(std::stoull(arg_value(argc, argv, "--max-payload", "8192")));
    const auto pool_count = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--pool-count", "64")));
    const auto buffer_size = static_cast<std::size_t>(std::stoull(
        arg_value(argc, argv, "--buffer-size", std::to_string(static_cast<std::size_t>(width) * height * avm::dds_shm_bytes_per_pixel(format)))));
    const auto startup_ms = static_cast<int>(std::stoi(arg_value(argc, argv, "--startup-ms", "5000")));
    const auto linger_ms = static_cast<int>(std::stoi(arg_value(argc, argv, "--linger-ms", "2000")));
    const auto warmup_frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--warmup-frames", "10")));
    const auto repeat = std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--repeat", "1"))));
    const auto inter_repeat_us = static_cast<int>(std::stoi(arg_value(argc, argv, "--inter-repeat-us", "1000")));
    const std::string output = arg_value(argc, argv, "--output", "logs/benchmark_v2_5_dds_shm_synthetic/camera_dds_shm_pub.csv");
    const bool reliable = has_flag(argc, argv, "--reliable");
    const bool allow_unavailable = has_flag(argc, argv, "--allow-unavailable");

    ensure_parent_dir(output);
    std::ofstream csv(output);
    write_pub_csv_header(csv);

    if (shm_name.empty() || shm_name.front() != '/') {
        std::cerr << "[camera_dds_shm_pub] --shm-name must start with '/'\n";
        csv << "pub," << avm::fastdds_support_compiled() << "," << source << "," << topic << "," << shm_name
            << "," << format << "," << width << "," << height << "," << frames << "," << warmup_frames
            << "," << repeat << "," << pool_count << "," << buffer_size << ",0,0,0,0,0,1,0,INVALID_SHM_NAME\n";
        return 2;
    }
    if (pool_count == 0U || buffer_size < static_cast<std::size_t>(width) * height * avm::dds_shm_bytes_per_pixel(format)) {
        std::cerr << "[camera_dds_shm_pub] invalid pool-count or buffer-size\n";
        csv << "pub," << avm::fastdds_support_compiled() << "," << source << "," << topic << "," << shm_name
            << "," << format << "," << width << "," << height << "," << frames << "," << warmup_frames
            << "," << repeat << "," << pool_count << "," << buffer_size << ",0,0,0,0,0,1,0,INVALID_POOL\n";
        return 2;
    }

    if (!avm::fastdds_support_compiled()) {
        std::cout << "[camera_dds_shm_pub] status=" << avm::fastdds_support_status() << "\n";
        csv << "pub,false," << source << "," << topic << "," << shm_name << "," << format << "," << width << "," << height
            << "," << frames << "," << warmup_frames << "," << repeat << "," << pool_count << "," << buffer_size
            << ",0,0,0,0,0,0,0,NOT_COMPILED\n";
        return allow_unavailable ? 0 : 2;
    }

    ShmFramePool pool;
    if (!pool.create(shm_name, pool_count, buffer_size)) {
        std::cerr << "[camera_dds_shm_pub] create SHM pool failed name=" << shm_name << "\n";
        csv << "pub,true," << source << "," << topic << "," << shm_name << "," << format << "," << width << "," << height
            << "," << frames << "," << warmup_frames << "," << repeat << "," << pool_count << "," << buffer_size
            << ",0,0,0,0,1,0,0,SHM_CREATE_FAILED\n";
        return 3;
    }

    avm::FastddsRtpsOptions options;
    options.topic = topic;
    options.domain_id = domain_id;
    options.depth = depth;
    options.max_payload_size = max_payload;
    options.reliable = reliable;
    options.participant_name = "autovision_camera_dds_shm_pub";

    std::string error;
    avm::FastddsRtpsBus bus(options);
    if (!bus.open(&error)) {
        std::cerr << "[camera_dds_shm_pub] open FastDDS failed: " << error << "\n";
        csv << "pub,true," << source << "," << topic << "," << shm_name << "," << format << "," << width << "," << height
            << "," << frames << "," << warmup_frames << "," << repeat << "," << pool_count << "," << buffer_size
            << ",0,0,0,0,0,1,0,OPEN_FAILED\n";
        return 4;
    }

    std::unique_ptr<SensorAdapter> adapter;
    if (source == "camera") {
        adapter = std::make_unique<CameraAdapterV4L2>(device, width, height, fps, camera_output_format(format));
        if (!adapter->open() || !adapter->start()) {
            std::cerr << "[camera_dds_shm_pub] camera open/start failed device=" << device << "\n";
            csv << "pub,true," << source << "," << topic << "," << shm_name << "," << format << "," << width << "," << height
                << "," << frames << "," << warmup_frames << "," << repeat << "," << pool_count << "," << buffer_size
                << ",0,0,0,0,0,1,0,CAMERA_OPEN_FAILED\n";
            return 5;
        }
    } else if (source != "synthetic") {
        std::cerr << "[camera_dds_shm_pub] unsupported --source " << source << "\n";
        csv << "pub,true," << source << "," << topic << "," << shm_name << "," << format << "," << width << "," << height
            << "," << frames << "," << warmup_frames << "," << repeat << "," << pool_count << "," << buffer_size
            << ",0,0,0,0,0,1,0,UNSUPPORTED_SOURCE\n";
        return 6;
    }

    if (startup_ms > 0) {
        std::cout << "[camera_dds_shm_pub] startup_ms=" << startup_ms << " waiting before first metadata publish\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(startup_ms));
    }

    std::uint64_t sent = 0;
    std::uint64_t tx_attempts = 0;
    std::uint64_t shm_write_errors = 0;
    std::uint64_t metadata_errors = 0;
    std::uint64_t payload_errors = 0;
    std::size_t last_metadata_bytes = 0;
    std::size_t last_raw_payload_bytes = 0;

    auto next_tick = std::chrono::steady_clock::now();
    const auto frame_period = std::chrono::microseconds(fps == 0 ? 0 : static_cast<int>(1000000U / fps));
    const auto repeat_gap = std::chrono::microseconds(std::max(0, inter_repeat_us));
    const std::uint64_t total_frames = frames + warmup_frames;

    for (std::uint64_t i = 0; i < total_frames; ++i) {
        const std::uint64_t seq = i + 1U;
        SensorFrame frame;
        if (source == "camera") {
            if (!adapter->readFrame(frame)) {
                std::cerr << "[camera_dds_shm_pub] readFrame failed at seq=" << seq << "\n";
                break;
            }
            frame.frame_id = seq;
        } else {
            frame = make_synthetic_camera_frame(seq, width, height, format);
        }

        const auto buffer_index = static_cast<std::uint32_t>((seq - 1U) % pool_count);
        if (!pool.write(buffer_index, frame.data.data(), frame.data.size())) {
            ++shm_write_errors;
            std::cerr << "[camera_dds_shm_pub] SHM write failed seq=" << seq << " buffer_index=" << buffer_index << "\n";
            continue;
        }

        auto desc = avm::make_shared_frame_descriptor(frame, shm_name, buffer_index, pool_count, buffer_size,
                                                      static_cast<std::uint32_t>(seq));
        auto msg = avm::make_shared_frame_descriptor_envelope(desc);
        last_metadata_bytes = msg.payload.size();
        last_raw_payload_bytes = frame.data.size();
        const auto check = avm::validate_transport_message(msg, avm::TransportQosProfile{max_payload, depth, true, reliable});
        if (!check.ok) {
            metadata_errors += check.size_errors;
            payload_errors += check.payload_errors;
            std::cerr << "[camera_dds_shm_pub] metadata validation failed seq=" << seq << ": " << check.reason << "\n";
            continue;
        }

        bool published_once = false;
        for (std::uint32_t r = 0; r < repeat; ++r) {
            if (!bus.publish(msg, &error)) {
                ++metadata_errors;
                std::cerr << "[camera_dds_shm_pub] publish failed seq=" << seq << " repeat=" << r << ": " << error << "\n";
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
            std::cout << "[camera_dds_shm_pub] seq=" << seq
                      << " slot=" << buffer_index
                      << " metadata_bytes=" << last_metadata_bytes
                      << " raw_payload_bytes=" << last_raw_payload_bytes
                      << " sent=" << sent
                      << " attempts=" << tx_attempts << "\n";
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
        std::cout << "[camera_dds_shm_pub] linger_ms=" << linger_ms << " waiting before close\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(linger_ms));
    }

    const auto stats = bus.stats();
    const bool pass = (sent == frames && shm_write_errors == 0 && metadata_errors == 0 && payload_errors == 0 && stats.dropped == 0);
    csv << "pub,true," << source << "," << topic << "," << shm_name << "," << format << "," << width << "," << height
        << "," << frames << "," << warmup_frames << "," << repeat << "," << pool_count << "," << buffer_size
        << "," << last_metadata_bytes << "," << last_raw_payload_bytes << "," << tx_attempts << "," << sent
        << "," << shm_write_errors << "," << metadata_errors << "," << payload_errors << ","
        << (pass ? "PASS" : "PARTIAL") << "\n";
    std::cout << "[camera_dds_shm_pub] done sent=" << sent
              << " tx_attempts=" << tx_attempts
              << " shm_write_errors=" << shm_write_errors
              << " metadata_errors=" << metadata_errors
              << " payload_errors=" << payload_errors
              << " dropped=" << stats.dropped
              << " result=" << (pass ? "PASS" : "PARTIAL") << "\n";
    return pass ? 0 : 1;
}
