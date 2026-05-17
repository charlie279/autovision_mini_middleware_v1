/**
 * @file local_pubsub_transport.hpp
 * @brief Dependency-free local pub/sub backend used to migrate reference transport four-link payload checks into AutoVision.
 */
#pragma once

#include "transport_message.hpp"

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

namespace avm {

struct LocalPubSubStats {
    std::uint64_t published = 0;
    std::uint64_t received = 0;
    std::uint64_t dropped = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::uint64_t lost = 0;
};

class LocalPubSubTransport {
public:
    LocalPubSubTransport(std::string channel_name, TransportQosProfile qos);

    bool publish(const TransportEnvelope& msg, std::string* error = nullptr);
    bool subscribe(TransportEnvelope& msg);

    LocalPubSubStats stats() const;
    std::size_t depth() const;
    const TransportQosProfile& qos() const { return qos_; }
    const std::string& channel_name() const { return channel_name_; }

private:
    std::string channel_name_;
    TransportQosProfile qos_;
    mutable std::mutex mutex_;
    std::deque<TransportEnvelope> queue_;
    LocalPubSubStats stats_{};
};

}  // namespace avm

