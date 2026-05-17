/**
 * @file 22_transport_pattern.cpp
 * @brief Interface-level test for Transport::createTransmitter/createReceiver pattern.
 */
#include "transport_endpoint.hpp"
#include "transport_message.hpp"

#include <iostream>

int main() {
    avm::Transport transport;
    avm::TransportRole role;
    role.channel_name = "examples/transport_pattern";
    role.backend = avm::TransportBackend::RTPS;
    role.qos.msg_size = 512U * 1024U;
    role.qos.depth = 4U;
    role.qos.drop_oldest = true;

    auto transmitter = transport.createTransmitter(role);
    auto receiver = transport.createReceiver(role);
    const auto msg = avm::make_lidar_frame(0, 10000, 16);

    std::string error;
    if (!transmitter->transmit(msg, &error)) {
        std::cerr << "transmit failed: " << error << "\n";
        return 1;
    }

    avm::TransportEnvelope rx;
    if (!receiver->receive(rx)) {
        std::cerr << "receive failed\n";
        return 1;
    }

    const auto stats = receiver->stats();
    const auto check = avm::validate_transport_message(rx, role.qos);
    const bool pass = check.ok && stats.lost == 0U;
    std::cout << "[22_transport_pattern] backend=" << avm::transport_backend_to_string(role.backend)
              << " tx=" << transmitter->type_name()
              << " rx=" << receiver->type_name()
              << " kind=" << avm::transport_kind_to_string(rx.kind)
              << " seq=" << rx.seq
              << " bytes=" << rx.payload.size()
              << " size_error=" << check.size_errors
              << " payload_error=" << check.payload_errors
              << " lost=" << stats.lost
              << " result=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}

