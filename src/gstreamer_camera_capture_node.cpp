/**
 * @file gstreamer_camera_capture_node.cpp
 * @brief V2.6 GStreamer camera capture demo: v4l2src/appsink -> SensorFrame -> CSV validation.
 */
#include "crc32.hpp"
#include "gstreamer_camera_adapter.hpp"
#include "gstreamer_pipeline_config.hpp"
#include "sensor_frame.hpp"
#include "time_utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
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

void ensure_parent_dir(const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

void write_header(std::ofstream& csv) {
    csv << "role,compiled,device,format,width,height,fps,frames_requested,captured,size_errors,payload_errors,mean_interval_ms,p95_interval_ms,last_crc32,status\n";
}

SensorFrame make_synthetic_frame(std::uint64_t seq, std::uint32_t width, std::uint32_t height, const std::string& format) {
    const auto normalized = avm::gst_normalized_format(format);
    SensorFrame frame;
    frame.frame_id = seq;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 0;
    frame.width = width;
    frame.height = height;
    frame.format = avm::gst_avm_format_code(normalized);
    frame.data_size = static_cast<std::uint32_t>(avm::gst_frame_payload_size(width, height, normalized));
    frame.stride_bytes = height == 0 ? 0 : frame.data_size / height;
    frame.data.resize(frame.data_size);
    const std::uint8_t salt = normalized == "rgb" ? 0x52U : (normalized == "nv12" ? 0x4EU : 0x59U);
    for (std::size_t i = 0; i < frame.data.size(); ++i) {
        frame.data[i] = static_cast<std::uint8_t>((i * 37U + seq * 11U + salt) & 0xFFU);
    }
    return frame;
}

double percentile95(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t idx = std::min(values.size() - 1U, static_cast<std::size_t>(values.size() * 95U / 100U));
    return values[idx];
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string source = arg_value(argc, argv, "--source", "gstreamer");
    const std::string device = arg_value(argc, argv, "--device", "/dev/video0");
    const auto width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", "640")));
    const auto height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", "480")));
    const auto fps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--fps", "30")));
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "300")));
    const std::string format = avm::gst_normalized_format(arg_value(argc, argv, "--format", "yuyv"));
    const std::string pipeline = arg_value(argc, argv, "--pipeline", "");
    const std::string output = arg_value(argc, argv, "--output", "logs/benchmark_v2_6_gstreamer_camera/gstreamer_camera_capture.csv");
    const bool use_dmabuf = has_flag(argc, argv, "--use-dmabuf");
    const bool allow_unavailable = has_flag(argc, argv, "--allow-unavailable");

    ensure_parent_dir(output);
    std::ofstream csv(output);
    write_header(csv);

    if (source != "gstreamer" && source != "synthetic") {
        std::cerr << "[gstreamer_camera_capture_node] unsupported --source " << source << "\n";
        csv << "capture," << GStreamerCameraAdapter::compiled() << "," << device << "," << format << "," << width << "," << height
            << "," << fps << "," << frames << ",0,1,0,0,0,0,UNSUPPORTED_SOURCE\n";
        return 2;
    }

    if (source == "gstreamer" && !GStreamerCameraAdapter::compiled()) {
        std::cout << "[gstreamer_camera_capture_node] status=" << GStreamerCameraAdapter::support_status() << "\n";
        csv << "capture,false," << device << "," << format << "," << width << "," << height << "," << fps
            << "," << frames << ",0,0,0,0,0,0,NOT_COMPILED\n";
        return allow_unavailable ? 0 : 2;
    }

    std::unique_ptr<SensorAdapter> adapter;
    if (source == "gstreamer") {
        auto gst = std::make_unique<GStreamerCameraAdapter>(device, width, height, fps, format, use_dmabuf, pipeline);
        std::cout << "[gstreamer_camera_capture_node] pipeline=" << gst->pipeline() << "\n";
        if (!gst->open() || !gst->start()) {
            csv << "capture,true," << device << "," << format << "," << width << "," << height << "," << fps
                << "," << frames << ",0,1,0,0,0,0,OPEN_FAILED\n";
            return 3;
        }
        adapter = std::move(gst);
    }

    std::uint64_t captured = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::uint32_t last_crc = 0;
    std::vector<double> intervals_ms;
    std::uint64_t last_ts = 0;
    const auto expected_size = avm::gst_frame_payload_size(width, height, format);

    for (std::uint64_t i = 0; i < frames; ++i) {
        SensorFrame frame;
        if (source == "synthetic") {
            frame = make_synthetic_frame(i + 1U, width, height, format);
            if (fps > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1000000U / fps));
            }
        } else if (!adapter->readFrame(frame)) {
            std::cerr << "[gstreamer_camera_capture_node] readFrame failed at index=" << i << "\n";
            break;
        }

        ++captured;
        if (frame.data.size() != static_cast<std::size_t>(frame.data_size) || frame.data.empty()) {
            ++payload_errors;
        }
        if (source == "synthetic" && frame.data.size() != expected_size) {
            ++size_errors;
        }
        last_crc = crc32_compute(frame.data.data(), frame.data.size());
        if (last_ts != 0U && frame.timestamp_ns > last_ts) {
            intervals_ms.push_back(avm::ns_to_ms(frame.timestamp_ns - last_ts));
        }
        last_ts = frame.timestamp_ns;
    }

    if (adapter) {
        adapter->stop();
    }

    const double mean = intervals_ms.empty() ? 0.0 : std::accumulate(intervals_ms.begin(), intervals_ms.end(), 0.0) / static_cast<double>(intervals_ms.size());
    const double p95 = percentile95(intervals_ms);
    const bool pass = captured == frames && size_errors == 0U && payload_errors == 0U;
    csv << "capture," << GStreamerCameraAdapter::compiled() << "," << device << "," << format << "," << width << "," << height
        << "," << fps << "," << frames << "," << captured << "," << size_errors << "," << payload_errors
        << "," << std::fixed << std::setprecision(3) << mean << "," << p95 << "," << last_crc << "," << (pass ? "PASS" : "PARTIAL") << "\n";

    std::cout << "[gstreamer_camera_capture_node] captured=" << captured
              << " size_errors=" << size_errors
              << " payload_errors=" << payload_errors
              << " mean_interval_ms=" << std::fixed << std::setprecision(3) << mean
              << " p95_interval_ms=" << p95
              << " result=" << (pass ? "PASS" : "PARTIAL") << " csv=" << output << "\n";
    return pass ? 0 : 1;
}
