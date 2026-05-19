/**
 * @file 25_dds_shm_frame_meta.cpp
 * @brief Validate V2.5 DDS+SHM metadata descriptor without requiring FastDDS runtime.
 */
#include "avm_config.hpp"
#include "crc32.hpp"
#include "dds_shm_frame_meta.hpp"
#include "sensor_frame.hpp"
#include "shm_frame_pool.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <vector>

namespace {
SensorFrame make_frame(std::uint64_t seq, std::uint32_t width, std::uint32_t height, const std::string& format) {
    SensorFrame frame;
    frame.frame_id = seq;
    frame.timestamp_ns = avm::now_us() * 1000U;
    frame.sensor_type = 0;
    frame.width = width;
    frame.height = height;
    frame.format = avm::dds_shm_format_code(format);
    frame.stride_bytes = width * avm::dds_shm_bytes_per_pixel(format);
    frame.data_size = frame.stride_bytes * height;
    frame.data.resize(frame.data_size);
    for (std::size_t i = 0; i < frame.data.size(); ++i) {
        frame.data[i] = static_cast<std::uint8_t>((i * 131U + seq * 17U + 0x25U) & 0xFFU);
    }
    return frame;
}
}  // namespace

int main() {
    const std::string shm_name = "/avm_example_v25_dds_shm_pool";
    const std::uint32_t width = 640;
    const std::uint32_t height = 480;
    const std::string format = "yuyv";
    const std::uint32_t pool_count = 4;
    const std::size_t buffer_size = static_cast<std::size_t>(width) * height * avm::dds_shm_bytes_per_pixel(format);

    ::shm_unlink(shm_name.c_str());
    ShmFramePool pool;
    if (!pool.create(shm_name, pool_count, buffer_size)) {
        std::cerr << "[25_dds_shm_frame_meta] create SHM pool failed\n";
        return 1;
    }

    std::uint64_t metadata_bytes = 0;
    std::uint64_t payload_errors = 0;
    std::uint64_t size_errors = 0;
    for (std::uint64_t seq = 1; seq <= 10; ++seq) {
        auto frame = make_frame(seq, width, height, format);
        const auto slot = static_cast<std::uint32_t>((seq - 1U) % pool_count);
        if (!pool.write(slot, frame.data.data(), frame.data.size())) {
            ++size_errors;
            continue;
        }
        const auto desc = avm::make_shared_frame_descriptor(frame, shm_name, slot, pool_count, buffer_size,
                                                            static_cast<std::uint32_t>(seq));
        auto msg = avm::make_shared_frame_descriptor_envelope(desc);
        metadata_bytes = msg.payload.size();

        avm::SharedFrameDescriptor decoded;
        std::string error;
        if (!avm::parse_shared_frame_descriptor_envelope(msg, decoded, &error)) {
            std::cerr << "[25_dds_shm_frame_meta] parse failed: " << error << "\n";
            ++payload_errors;
            continue;
        }
        std::vector<std::uint8_t> copied(static_cast<std::size_t>(decoded.payload_size));
        if (!pool.read(decoded.buffer_index, copied.data(), copied.size())) {
            ++size_errors;
            continue;
        }
        if (copied.size() != frame.data.size()) {
            ++size_errors;
        }
        const auto crc = crc32_compute(copied.data(), copied.size());
        if (crc != decoded.payload_crc32) {
            ++payload_errors;
        }
    }

    pool.close();
    ::shm_unlink(shm_name.c_str());
    const bool pass = (metadata_bytes > 0 && size_errors == 0 && payload_errors == 0);
    std::cout << "[25_dds_shm_frame_meta] metadata_bytes=" << metadata_bytes
              << " raw_payload_bytes=" << buffer_size
              << " size_errors=" << size_errors
              << " payload_errors=" << payload_errors
              << " result=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
