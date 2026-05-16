/**
 * @file transport_message.cpp
 * @brief CamMW-style four-link transport message generation and validation.
 */
#include "transport_message.hpp"

#include "crc32.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace avm {

const char* transport_kind_to_string(TransportMessageKind kind) {
    switch (kind) {
        case TransportMessageKind::TEST:
            return "test";
        case TransportMessageKind::RAW_FRAME:
            return "raw";
        case TransportMessageKind::ENCODED_FRAME:
            return "encoded";
        case TransportMessageKind::LIDAR_FRAME:
            return "lidar";
        default:
            return "unknown";
    }
}

std::uint64_t now_us() {
    using Clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count());
}

std::uint32_t compute_transport_crc(const std::vector<std::uint8_t>& payload) {
    return crc32_compute(payload.data(), payload.size());
}

namespace {
void fill_pattern(std::vector<std::uint8_t>& payload, std::uint64_t seq, std::uint8_t salt) {
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>((i * 131U + seq * 17U + salt) & 0xFFU);
    }
}

TransportEnvelope finalize(TransportEnvelope msg) {
    msg.timestamp_us = now_us();
    msg.payload_crc32 = compute_transport_crc(msg.payload);
    return msg;
}
}  // namespace

TransportEnvelope make_test_msg(std::uint64_t seq) {
    TransportEnvelope msg;
    msg.kind = TransportMessageKind::TEST;
    msg.seq = seq;
    const std::string text = "AutoVision-CamMW-TestMsg seq=" + std::to_string(seq);
    msg.payload.assign(text.begin(), text.end());
    msg.raw_size = static_cast<std::uint32_t>(msg.payload.size());
    return finalize(std::move(msg));
}

TransportEnvelope make_raw_frame(std::uint64_t seq, std::uint32_t width,
                                 std::uint32_t height, std::uint32_t channels) {
    TransportEnvelope msg;
    msg.kind = TransportMessageKind::RAW_FRAME;
    msg.seq = seq;
    msg.width = width;
    msg.height = height;
    msg.channels = channels;
    const std::size_t bytes = static_cast<std::size_t>(width) * height * channels;
    msg.payload.resize(bytes);
    fill_pattern(msg.payload, seq, 0x52U);
    msg.raw_size = static_cast<std::uint32_t>(bytes);
    return finalize(std::move(msg));
}

TransportEnvelope make_encoded_frame(std::uint64_t seq, std::uint32_t width,
                                     std::uint32_t height, double ratio) {
    TransportEnvelope msg;
    msg.kind = TransportMessageKind::ENCODED_FRAME;
    msg.seq = seq;
    msg.width = width;
    msg.height = height;
    msg.channels = 1;
    const std::size_t raw = static_cast<std::size_t>(width) * height;
    const auto encoded = static_cast<std::size_t>(std::max(1.0, std::round(static_cast<double>(raw) * ratio)));
    msg.payload.resize(encoded);
    fill_pattern(msg.payload, seq, 0x45U);
    msg.raw_size = static_cast<std::uint32_t>(raw);
    msg.encoded_size = static_cast<std::uint32_t>(encoded);
    return finalize(std::move(msg));
}

TransportEnvelope make_lidar_frame(std::uint64_t seq, std::uint32_t point_count,
                                   std::uint32_t point_stride) {
    TransportEnvelope msg;
    msg.kind = TransportMessageKind::LIDAR_FRAME;
    msg.seq = seq;
    msg.point_count = point_count;
    msg.point_stride = point_stride;
    const std::size_t bytes = static_cast<std::size_t>(point_count) * point_stride;
    msg.payload.resize(bytes);
    fill_pattern(msg.payload, seq, 0x4CU);
    msg.raw_size = static_cast<std::uint32_t>(bytes);
    return finalize(std::move(msg));
}

std::size_t expected_transport_payload_size(const TransportEnvelope& msg) {
    switch (msg.kind) {
        case TransportMessageKind::TEST:
            return msg.payload.size();
        case TransportMessageKind::RAW_FRAME:
            return static_cast<std::size_t>(msg.width) * msg.height * msg.channels;
        case TransportMessageKind::ENCODED_FRAME:
            return msg.encoded_size;
        case TransportMessageKind::LIDAR_FRAME:
            return static_cast<std::size_t>(msg.point_count) * msg.point_stride;
        default:
            return msg.payload.size();
    }
}

TransportValidationResult validate_transport_message(const TransportEnvelope& msg,
                                                     const TransportQosProfile& qos) {
    TransportValidationResult result;
    const std::size_t expected = expected_transport_payload_size(msg);
    if (msg.payload.size() != expected) {
        result.ok = false;
        result.size_errors = 1;
        std::ostringstream oss;
        oss << "payload size mismatch expected=" << expected << " actual=" << msg.payload.size();
        result.reason = oss.str();
        return result;
    }
    if (msg.payload.size() > qos.msg_size) {
        result.ok = false;
        result.size_errors = 1;
        std::ostringstream oss;
        oss << "payload exceeds qos.msg_size payload=" << msg.payload.size() << " qos=" << qos.msg_size;
        result.reason = oss.str();
        return result;
    }
    const std::uint32_t crc = compute_transport_crc(msg.payload);
    if (crc != msg.payload_crc32) {
        result.ok = false;
        result.payload_errors = 1;
        std::ostringstream oss;
        oss << "payload crc mismatch expected=0x" << std::hex << msg.payload_crc32 << " actual=0x" << crc;
        result.reason = oss.str();
        return result;
    }
    result.reason = "OK";
    return result;
}

}  // namespace avm
