/**
 * @file media_node.cpp
 * @brief 生产者节点：读取传感器帧，将 payload 写入 FramePool，并将 FrameMeta 推入 Ring。
 */
#include "avm_config.hpp"
#include "camera_adapter_v4l2.hpp"
#include "crc32.hpp"
#include "file_adapter.hpp"
#include "frame_meta.hpp"
#include "lidar_sim_adapter.hpp"
#include "record_replay.hpp"
#include "shared_status.hpp"
#include "udp_lidar_sim_adapter.hpp"
#include "shm_frame_pool.hpp"
#include "shm_ring_buffer.hpp"

#include <chrono>
#include <cstdint>
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

const char* format_name(std::uint32_t format) {
    if (format == avm::kFormatRgb888) {
        return "RGB888";
    }
    if (format == avm::kFormatYuyv) {
        return "YUYV";
    }
    if (format == avm::kFormatLidarFloat32) {
        return "LI32";
    }
    return "UNKNOWN";
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string source = arg_value(argc, argv, "--source", "file");
    const std::string input = arg_value(argc, argv, "--input", "assets/input_640x480_rgb.raw");
    const std::string device = arg_value(argc, argv, "--device", "/dev/video0");
    const int frames = std::stoi(arg_value(argc, argv, "--frames", "120"));
    const auto width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", "640")));
    const auto height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", "480")));
    const auto camera_fps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--fps", "30")));
    const std::string camera_output = arg_value(argc, argv, "--camera-output", "rgb");
    const std::string replay_path = arg_value(argc, argv, "--replay", "logs/record.avmr");
    const std::string record_path = arg_value(argc, argv, "--record", "");
    const std::string drop_policy = arg_value(argc, argv, "--drop-policy", "block");
    const auto udp_port = static_cast<std::uint16_t>(std::stoul(arg_value(argc, argv, "--udp-port", "2368")));
    const bool inject_bad_crc = has_flag(argc, argv, "--inject-bad-crc");
    const bool inject_frame_jump = has_flag(argc, argv, "--inject-frame-jump");
    const bool inject_alive_jump = has_flag(argc, argv, "--inject-alive-jump");

    std::unique_ptr<SensorAdapter> adapter;
    if (source == "file") {
        adapter = std::make_unique<FileAdapter>(input, width, height, avm::kDefaultChannels);
    } else if (source == "lidar_sim") {
        adapter = std::make_unique<LidarSimAdapter>();
    } else if (source == "udp_lidar") {
        adapter = std::make_unique<UdpLidarSimAdapter>("0.0.0.0", udp_port);
    } else if (source == "replay") {
        adapter = std::make_unique<avm::FrameReplayAdapter>(replay_path);
    } else if (source == "camera") {
        const CameraOutputFormat output_format =
            (camera_output == "yuyv") ? CameraOutputFormat::YUYV : CameraOutputFormat::RGB888;
        adapter = std::make_unique<CameraAdapterV4L2>(device, width, height, camera_fps, output_format);
    } else {
        std::cerr << "[media_node] unsupported source: " << source << "\n";
        return 1;
    }

    SharedStatus status;
    status.create_or_open(avm::kStatusName);

    ShmFramePool frame_pool;
    if (!frame_pool.create(avm::kFramePoolName, avm::kFramePoolCount, avm::kFrameBufferSize)) {
        return 2;
    }

    ShmRingBuffer frame_ring;
    if (!frame_ring.create(avm::kFrameMetaRingName, sizeof(FrameMeta), avm::kRingCapacity)) {
        return 3;
    }

    if (!adapter->open() || !adapter->start()) {
        std::cerr << "[media_node] adapter open/start failed\n";
        return 4;
    }

    std::uint32_t alive_counter = 0;
    std::uint64_t produced = 0;
    avm::FrameRecorder recorder;
    const bool record_enabled = !record_path.empty();
    if (record_enabled && !recorder.open(record_path)) {
        return 5;
    }
    auto next_tick = std::chrono::steady_clock::now();

    for (int i = 0; i < frames; ++i) {
        SensorFrame sensor_frame;
        if (!adapter->readFrame(sensor_frame)) {
            std::cerr << "[media_node] readFrame failed at i=" << i << "\n";
            break;
        }

        if (sensor_frame.data_size > avm::kFrameBufferSize) {
            std::cerr << "[media_node] frame too large: " << sensor_frame.data_size << "\n";
            break;
        }

        FrameMeta meta;
        meta.frame_id = sensor_frame.frame_id;
        if (inject_frame_jump && i == 40) {
            meta.frame_id += 100;
        }
        meta.timestamp_ns = sensor_frame.timestamp_ns;
        meta.sensor_type = sensor_frame.sensor_type;
        meta.width = sensor_frame.width;
        meta.height = sensor_frame.height;
        meta.format = sensor_frame.format;
        meta.data_size = sensor_frame.data_size;
        meta.stride_bytes = sensor_frame.stride_bytes;
        meta.buffer_index = static_cast<std::uint32_t>(meta.frame_id % avm::kFramePoolCount);
        meta.crc32 = crc32_compute(sensor_frame.data.data(), sensor_frame.data.size());
        if (inject_bad_crc && i == 50) {
            meta.crc32 ^= 0x00FF00FFU;
        }
        meta.alive_counter = ++alive_counter;
        if (inject_alive_jump && i == 30) {
            meta.alive_counter += 2;
            alive_counter = meta.alive_counter;
        }
        meta.status = 0;

        if (!frame_pool.write(meta.buffer_index, sensor_frame.data.data(), sensor_frame.data.size())) {
            std::cerr << "[media_node] frame_pool.write failed\n";
            break;
        }
        if (record_enabled) {
            recorder.write(sensor_frame, meta.crc32);
        }
        const bool pushed = (drop_policy == "drop_oldest")
            ? frame_ring.try_push_drop_oldest(&meta, sizeof(meta))
            : frame_ring.push(&meta, sizeof(meta));
        if (!pushed) {
            std::cerr << "[media_node] frame_ring.push failed\n";
            break;
        }

        ++produced;
        status.update_node("media", produced, meta.frame_id);

        if (produced == 1 || produced % 30 == 0) {
            std::cout << "[media_node] frame_id=" << meta.frame_id
                      << " sensor_type=" << meta.sensor_type
                      << " format=" << format_name(meta.format)
                      << " size=" << meta.data_size
                      << " stride=" << meta.stride_bytes
                      << " alive=" << meta.alive_counter
                      << " q_depth=" << frame_ring.depth() << "/" << frame_ring.capacity()
                      << " drops=" << frame_ring.drop_count()
                      << " buffer_index=" << meta.buffer_index << "\n";
        }

        if (source != "camera") {
            std::uint32_t fps = status.desired_fps();
            if (fps == 0) {
                fps = avm::kDefaultFps;
            }
            next_tick += std::chrono::milliseconds(1000 / static_cast<int>(fps));
            std::this_thread::sleep_until(next_tick);
        }
    }

    adapter->stop();
    recorder.close();
    status.update_node("media", produced, produced, ErrorCode::OK, 0, 0, 0, frame_ring.drop_count());
    std::cout << "[media_node] finished produced=" << produced
              << " queue_drops=" << frame_ring.drop_count()
              << " recorded=" << (record_enabled ? recorder.count() : 0) << "\n";
    return 0;
}
