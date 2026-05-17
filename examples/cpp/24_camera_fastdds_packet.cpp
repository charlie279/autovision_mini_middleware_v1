/**
 * @file 24_camera_fastdds_packet.cpp
 * @brief V2.4 interface-level check for 640x480 camera raw frame packet size and QoS boundary.
 */
#include "fastdds_rtps_transport.hpp"
#include "transport_message.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
avm::TransportEnvelope make_camera_raw(std::uint64_t seq, std::uint32_t width, std::uint32_t height, std::uint32_t bytes_per_pixel) {
    avm::TransportEnvelope msg;
    msg.kind = avm::TransportMessageKind::RAW_FRAME;
    msg.seq = seq;
    msg.timestamp_us = avm::now_us();
    msg.width = width;
    msg.height = height;
    msg.channels = bytes_per_pixel;
    msg.raw_size = width * height * bytes_per_pixel;
    msg.payload.resize(msg.raw_size);
    for (std::size_t i = 0; i < msg.payload.size(); ++i) {
        msg.payload[i] = static_cast<std::uint8_t>((i * 131U + seq * 17U + 0x24U) & 0xFFU);
    }
    msg.payload_crc32 = avm::compute_transport_crc(msg.payload);
    return msg;
}
}

int main() {
    const auto yuyv = make_camera_raw(24, 640, 480, 2);
    const auto rgb = make_camera_raw(25, 640, 480, 3);

    const auto yuyv_bytes = avm::serialize_transport_envelope(yuyv);
    const auto rgb_bytes = avm::serialize_transport_envelope(rgb);

    avm::TransportEnvelope yuyv_decoded;
    avm::TransportEnvelope rgb_decoded;
    std::string error;
    const bool yuyv_decode_ok = avm::deserialize_transport_envelope(yuyv_bytes, yuyv_decoded, &error);
    const bool rgb_decode_ok = avm::deserialize_transport_envelope(rgb_bytes, rgb_decoded, &error);

    avm::TransportQosProfile v24_qos;
    v24_qos.msg_size = 2U * 1024U * 1024U;
    v24_qos.depth = 8;

    avm::TransportQosProfile old_qos;
    old_qos.msg_size = 512U * 1024U;
    old_qos.depth = 8;

    const auto yuyv_check = avm::validate_transport_message(yuyv_decoded, v24_qos);
    const auto rgb_check = avm::validate_transport_message(rgb_decoded, v24_qos);
    const auto old_check = avm::validate_transport_message(yuyv_decoded, old_qos);

    const bool pass = yuyv_decode_ok && rgb_decode_ok && yuyv_check.ok && rgb_check.ok && !old_check.ok;
    std::cout << "[24_camera_fastdds_packet] fastdds_status=" << avm::fastdds_support_status() << "\n";
    std::cout << "[24_camera_fastdds_packet] yuyv_payload=" << yuyv.payload.size()
              << " yuyv_packet=" << yuyv_bytes.size()
              << " rgb_payload=" << rgb.payload.size()
              << " rgb_packet=" << rgb_bytes.size()
              << " v24_qos_yuyv=" << (yuyv_check.ok ? "OK" : yuyv_check.reason)
              << " v24_qos_rgb=" << (rgb_check.ok ? "OK" : rgb_check.reason)
              << " old_512k_qos=" << (old_check.ok ? "UNEXPECTED_OK" : "EXPECTED_FAIL")
              << " result=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
