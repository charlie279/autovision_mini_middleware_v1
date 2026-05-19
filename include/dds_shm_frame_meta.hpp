/**
 * @file dds_shm_frame_meta.hpp
 * @brief V2.5 DDS+SHM mixed-route descriptor and serialization helpers.
 *
 * V2.5 does not send raw camera pixels through DDS. DDS carries only a compact
 * SharedFrameDescriptor, while POSIX SHM carries the large raw payload.
 */
#pragma once

#include "sensor_frame.hpp"
#include "transport_message.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace avm {

constexpr std::uint32_t kDdsShmMetaMagic = 0x4156534DU;  // "AVSM"
constexpr std::uint32_t kDdsShmMetaVersion = 1U;
constexpr std::size_t kDdsShmNameMax = 128U;

struct SharedFrameDescriptor {
    std::uint64_t seq = 0;
    std::uint64_t timestamp_us = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t format = 0;
    std::uint32_t bytes_per_pixel = 0;
    std::uint64_t payload_size = 0;
    std::uint32_t payload_crc32 = 0;
    std::uint32_t buffer_index = 0;
    std::uint32_t buffer_count = 0;
    std::uint64_t buffer_size = 0;
    std::uint64_t offset = 0;
    std::uint32_t alive_counter = 0;
    char shm_name[kDdsShmNameMax] = {0};
};

std::string dds_shm_normalized_format(const std::string& format);
std::uint32_t dds_shm_bytes_per_pixel(const std::string& format);
std::uint32_t dds_shm_format_code(const std::string& format);
std::string dds_shm_format_to_string(std::uint32_t format_code);
std::size_t dds_shm_expected_payload_size(const SharedFrameDescriptor& desc);

std::vector<std::uint8_t> serialize_shared_frame_descriptor(const SharedFrameDescriptor& desc);
bool deserialize_shared_frame_descriptor(const std::vector<std::uint8_t>& bytes,
                                         SharedFrameDescriptor& desc,
                                         std::string* error = nullptr);

TransportEnvelope make_shared_frame_descriptor_envelope(const SharedFrameDescriptor& desc);
bool parse_shared_frame_descriptor_envelope(const TransportEnvelope& msg,
                                            SharedFrameDescriptor& desc,
                                            std::string* error = nullptr);

SharedFrameDescriptor make_shared_frame_descriptor(const SensorFrame& frame,
                                                   const std::string& shm_name,
                                                   std::uint32_t buffer_index,
                                                   std::uint32_t buffer_count,
                                                   std::uint64_t buffer_size,
                                                   std::uint32_t alive_counter);

}  // namespace avm
