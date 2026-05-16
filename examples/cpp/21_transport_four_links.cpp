/**
 * @file 21_transport_four_links.cpp
 * @brief Interface-level test for CamMW-style local pub/sub large-payload validation.
 */
#include "local_pubsub_transport.hpp"
#include "transport_message.hpp"

#include <iostream>

int main() {
    avm::TransportQosProfile qos;
    qos.msg_size = 512U * 1024U;
    qos.depth = 4U;
    qos.drop_oldest = true;

    avm::LocalPubSubTransport bus("examples/transport_four_links", qos);
    const auto raw = avm::make_raw_frame(1, 640, 480, 1);
    std::string error;
    if (!bus.publish(raw, &error)) {
        std::cerr << "publish failed: " << error << "\n";
        return 1;
    }

    avm::TransportEnvelope rx;
    if (!bus.subscribe(rx)) {
        std::cerr << "subscribe failed\n";
        return 1;
    }

    const auto result = avm::validate_transport_message(rx, qos);
    std::cout << "[21_transport_four_links] kind=" << avm::transport_kind_to_string(rx.kind)
              << " seq=" << rx.seq
              << " bytes=" << rx.payload.size()
              << " size_error=" << result.size_errors
              << " payload_error=" << result.payload_errors
              << " result=" << (result.ok ? "PASS" : "FAIL") << "\n";
    return result.ok ? 0 : 1;
}
