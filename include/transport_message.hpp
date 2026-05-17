/**
 * @file transport_message.hpp
 * @brief reference transport-style four-link transport message schema and payload validation helpers.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace avm {

enum class TransportMessageKind : std::uint32_t {
    TEST = 0,
    RAW_FRAME = 1,
    ENCODED_FRAME = 2,
    LIDAR_FRAME = 3
};

struct TransportQosProfile {
    std::size_t msg_size = 512U * 1024U;
    std::size_t depth = 8U;
    bool drop_oldest = true;
    bool reliable = false;
};

struct TransportEnvelope {
    TransportMessageKind kind = TransportMessageKind::TEST;
    std::uint64_t seq = 0;
    std::uint64_t timestamp_us = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = 0;
    std::uint32_t point_count = 0;
    std::uint32_t point_stride = 0;
    std::uint32_t raw_size = 0;
    std::uint32_t encoded_size = 0;
    std::uint32_t payload_crc32 = 0;
    std::vector<std::uint8_t> payload;
};

struct TransportValidationResult {
    bool ok = true;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::string reason = "OK";
};

const char* transport_kind_to_string(TransportMessageKind kind);
std::uint64_t now_us();
std::uint32_t compute_transport_crc(const std::vector<std::uint8_t>& payload);

TransportEnvelope make_test_msg(std::uint64_t seq);
TransportEnvelope make_raw_frame(std::uint64_t seq, std::uint32_t width,
                                 std::uint32_t height, std::uint32_t channels);
TransportEnvelope make_encoded_frame(std::uint64_t seq, std::uint32_t width,
                                     std::uint32_t height, double ratio);
TransportEnvelope make_lidar_frame(std::uint64_t seq, std::uint32_t point_count,
                                   std::uint32_t point_stride);

TransportValidationResult validate_transport_message(const TransportEnvelope& msg,
                                                     const TransportQosProfile& qos);

std::size_t expected_transport_payload_size(const TransportEnvelope& msg);

}  // namespace avm
