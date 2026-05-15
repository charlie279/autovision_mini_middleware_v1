/**
 * @file preprocess_node.cpp
 * @brief 前处理节点：读取 FrameMeta 和 payload，做 CRC、frame_id 检查、RGB/YUYV 前处理，输出 TensorMeta。
 */
#include "avm_config.hpp"
#include "crc32.hpp"
#include "frame_meta.hpp"
#include "image_preprocess.hpp"
#include "latency_profiler.hpp"
#include "shared_status.hpp"
#include "shm_frame_pool.hpp"
#include "shm_ring_buffer.hpp"
#include "tensor_meta.hpp"
#include "time_utils.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
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

template <typename OpenFunc>
bool retry_open(OpenFunc&& open_func, const char* name, int retry_count = 60) {
    for (int i = 0; i < retry_count; ++i) {
        if (open_func()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cerr << "[preprocess_node] failed to open " << name << "\n";
    return false;
}

void save_ppm_preview(const std::vector<std::uint8_t>& rgb,
                      std::uint32_t width,
                      std::uint32_t height,
                      const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        return;
    }
    out << "P6\n" << width << " " << height << "\n255\n";
    out.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const int frames = std::stoi(arg_value(argc, argv, "--frames", "120"));
    const int save_every = std::stoi(arg_value(argc, argv, "--save-every", "60"));
    std::filesystem::create_directories("logs/preprocess");

    SharedStatus status;
    status.create_or_open(avm::kStatusName);

    ShmFramePool frame_pool;
    ShmRingBuffer frame_ring;
    ShmFramePool tensor_pool;
    ShmRingBuffer tensor_ring;

    if (!retry_open([&]() { return frame_pool.open(avm::kFramePoolName, avm::kFramePoolCount, avm::kFrameBufferSize); },
                    "frame_pool")) {
        return 1;
    }
    if (!retry_open([&]() { return frame_ring.open(avm::kFrameMetaRingName, sizeof(FrameMeta), avm::kRingCapacity); },
                    "frame_meta_ring")) {
        return 2;
    }
    if (!tensor_pool.create(avm::kTensorPoolName, avm::kTensorPoolCount, avm::kTensorBufferSize)) {
        return 3;
    }
    if (!tensor_ring.create(avm::kTensorMetaRingName, sizeof(TensorMeta), avm::kRingCapacity)) {
        return 4;
    }

    LatencyProfiler profiler;
    std::uint64_t processed = 0;
    std::uint64_t crc_errors = 0;
    std::uint64_t frame_jumps = 0;
    std::uint64_t unsupported_format = 0;
    std::uint64_t last_frame_id = 0;

    std::vector<std::uint8_t> raw_buffer;
    std::vector<float> tensor_payload(static_cast<std::size_t>(avm::kTensorWidth) * avm::kTensorHeight * avm::kTensorChannels);

    for (int i = 0; i < frames; ++i) {
        FrameMeta meta;
        if (!frame_ring.pop(&meta, sizeof(meta), 3000)) {
            std::cerr << "[preprocess_node] timeout waiting FrameMeta\n";
            status.update_node("preprocess", processed, last_frame_id,
                               ErrorCode::HEARTBEAT_TIMEOUT, crc_errors, frame_jumps);
            continue;
        }

        const std::uint64_t t0 = avm::now_ns();
        if (raw_buffer.size() < meta.data_size) {
            raw_buffer.resize(meta.data_size);
        }
        if (!frame_pool.read(meta.buffer_index, raw_buffer.data(), meta.data_size)) {
            std::cerr << "[preprocess_node] frame_pool.read failed\n";
            continue;
        }

        const std::uint32_t actual_crc = crc32_compute(raw_buffer.data(), meta.data_size);
        ErrorCode error_code = ErrorCode::OK;
        if (actual_crc != meta.crc32) {
            ++crc_errors;
            error_code = ErrorCode::CRC_ERROR;
            std::cerr << "[preprocess_node] CRC error frame_id=" << meta.frame_id << "\n";
        }

        if (last_frame_id != 0 && meta.frame_id != last_frame_id + 1) {
            ++frame_jumps;
            error_code = ErrorCode::FRAME_ID_JUMP;
            std::cerr << "[preprocess_node] frame jump last=" << last_frame_id
                      << " current=" << meta.frame_id << "\n";
        }
        last_frame_id = meta.frame_id;

        if (meta.sensor_type != 0 && meta.sensor_type != 2) {
            status.update_node("preprocess", processed, meta.frame_id, error_code, crc_errors, frame_jumps);
            continue;
        }

        const std::uint64_t t_pre_start = avm::now_ns();
        const std::uint32_t stride = meta.stride_bytes;
        if (meta.format == avm::kFormatRgb888) {
            avm::resize_rgb888_to_tensor(raw_buffer.data(), meta.width, meta.height,
                                         stride == 0 ? meta.width * 3U : stride,
                                         avm::kTensorWidth, avm::kTensorHeight,
                                         tensor_payload.data());
        } else if (meta.format == avm::kFormatYuyv) {
            avm::resize_yuyv_to_tensor(raw_buffer.data(), meta.width, meta.height,
                                       stride == 0 ? meta.width * 2U : stride,
                                       avm::kTensorWidth, avm::kTensorHeight,
                                       tensor_payload.data());
        } else {
            ++unsupported_format;
            std::cerr << "[preprocess_node] unsupported format=" << meta.format
                      << " frame_id=" << meta.frame_id << "\n";
            status.update_node("preprocess", processed, meta.frame_id, error_code, crc_errors, frame_jumps);
            continue;
        }
        const std::uint64_t t_pre_end = avm::now_ns();

        TensorMeta tensor;
        tensor.frame_id = meta.frame_id;
        tensor.timestamp_ns = avm::now_ns();
        tensor.width = avm::kTensorWidth;
        tensor.height = avm::kTensorHeight;
        tensor.channels = avm::kTensorChannels;
        tensor.data_type = 1;
        tensor.data_size = static_cast<std::uint32_t>(tensor_payload.size() * sizeof(float));
        tensor.buffer_index = static_cast<std::uint32_t>(tensor.frame_id % avm::kTensorPoolCount);

        if (!tensor_pool.write(tensor.buffer_index, tensor_payload.data(), tensor.data_size)) {
            std::cerr << "[preprocess_node] tensor_pool.write failed\n";
            continue;
        }
        if (!tensor_ring.push(&tensor, sizeof(tensor))) {
            std::cerr << "[preprocess_node] tensor_ring.push failed\n";
            continue;
        }

        ++processed;
        const double total_ms = avm::ns_to_ms(avm::now_ns() - t0);
        profiler.add(total_ms);
        status.update_node("preprocess", processed, meta.frame_id, error_code, crc_errors, frame_jumps);

        if (save_every > 0 && meta.format == avm::kFormatRgb888 &&
            processed % static_cast<std::uint64_t>(save_every) == 0) {
            std::vector<std::uint8_t> preview(raw_buffer.begin(), raw_buffer.begin() + meta.data_size);
            save_ppm_preview(preview, meta.width, meta.height,
                             "logs/preprocess/frame_" + std::to_string(processed) + ".ppm");
        }

        if (processed == 1 || processed % 30 == 0) {
            std::cout << "[Preprocess] frame_id=" << meta.frame_id
                      << " input_format=" << (meta.format == avm::kFormatYuyv ? "YUYV" : "RGB888")
                      << " preprocess=" << avm::ns_to_ms(t_pre_end - t_pre_start) << "ms"
                      << " total=" << total_ms << "ms"
                      << " mean=" << profiler.mean() << "ms"
                      << " p95=" << profiler.percentile(95.0) << "ms\n";
        }
    }

    std::cout << "[preprocess_node] finished processed=" << processed
              << " crc_errors=" << crc_errors
              << " frame_jumps=" << frame_jumps
              << " unsupported_format=" << unsupported_format << "\n";
    return 0;
}
