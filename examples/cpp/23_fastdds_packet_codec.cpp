/**
 * @file 23_fastdds_packet_codec.cpp
 * @brief Interface-level check for the AutoVision FastDDS packet codec.
 */
#include "fastdds_rtps_transport.hpp"
#include "transport_message.hpp"

#include <iostream>

int main() {
    auto msg = avm::make_lidar_frame(7, 10000, 16);
    const auto bytes = avm::serialize_transport_envelope(msg);
    avm::TransportEnvelope decoded;
    std::string error;
    const bool ok = avm::deserialize_transport_envelope(bytes, decoded, &error);
    const auto check = avm::validate_transport_message(decoded, avm::TransportQosProfile{});
    std::cout << "[23_fastdds_packet_codec] fastdds_status=" << avm::fastdds_support_status() << "\n";
    std::cout << "[23_fastdds_packet_codec] encoded_bytes=" << bytes.size()
              << " kind=" << avm::transport_kind_to_string(decoded.kind)
              << " seq=" << decoded.seq
              << " payload=" << decoded.payload.size()
              << " decode=" << (ok ? "OK" : error)
              << " validate=" << (check.ok ? "OK" : check.reason)
              << " result=" << ((ok && check.ok) ? "PASS" : "FAIL") << "\n";
    return (ok && check.ok) ? 0 : 1;
}

