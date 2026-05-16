/**
 * @file transport_endpoint.cpp
 * @brief Implementation of CamMW-style Transport / Transmitter / Dispatcher / Receiver abstraction.
 */
#include "transport_endpoint.hpp"

#include <algorithm>
#include <cctype>

namespace avm {

const char* transport_backend_to_string(TransportBackend backend) {
    switch (backend) {
        case TransportBackend::RTPS:
            return "rtps";
        case TransportBackend::SHM:
            return "shm";
        default:
            return "unknown";
    }
}

TransportBackend parse_transport_backend(const std::string& text, TransportBackend fallback) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lower == "rtps" || lower == "fastrtps" || lower == "dds") {
        return TransportBackend::RTPS;
    }
    if (lower == "shm" || lower == "shared_memory" || lower == "shared-memory") {
        return TransportBackend::SHM;
    }
    return fallback;
}

Transmitter::Transmitter(TransportRole role) : role_(std::move(role)) {}

RtpsTransmitter::RtpsTransmitter(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel)
    : Transmitter(std::move(role)), channel_(std::move(channel)) {}

bool RtpsTransmitter::transmit(const TransportEnvelope& msg, std::string* error) {
    return channel_ != nullptr && channel_->publish(msg, error);
}

TransportEndpointStats RtpsTransmitter::stats() const {
    TransportEndpointStats out;
    if (channel_ == nullptr) {
        return out;
    }
    const auto s = channel_->stats();
    out.sent = s.published;
    out.received = s.received;
    out.dropped = s.dropped;
    out.size_errors = s.size_errors;
    out.payload_errors = s.payload_errors;
    out.lost = s.lost;
    return out;
}

ShmTransmitter::ShmTransmitter(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel)
    : Transmitter(std::move(role)), channel_(std::move(channel)) {}

bool ShmTransmitter::transmit(const TransportEnvelope& msg, std::string* error) {
    return channel_ != nullptr && channel_->publish(msg, error);
}

TransportEndpointStats ShmTransmitter::stats() const {
    TransportEndpointStats out;
    if (channel_ == nullptr) {
        return out;
    }
    const auto s = channel_->stats();
    out.sent = s.published;
    out.received = s.received;
    out.dropped = s.dropped;
    out.size_errors = s.size_errors;
    out.payload_errors = s.payload_errors;
    out.lost = s.lost;
    return out;
}

Dispatcher::Dispatcher(TransportRole role) : role_(std::move(role)) {}

RtpsDispatcher::RtpsDispatcher(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel)
    : Dispatcher(std::move(role)), channel_(std::move(channel)) {}

bool RtpsDispatcher::dispatch(TransportEnvelope& msg) {
    if (channel_ == nullptr || !channel_->subscribe(msg)) {
        ++stats_.empty_polls;
        return false;
    }
    ++stats_.received;
    return true;
}

TransportEndpointStats RtpsDispatcher::stats() const {
    return stats_;
}

ShmDispatcher::ShmDispatcher(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel)
    : Dispatcher(std::move(role)), channel_(std::move(channel)) {}

bool ShmDispatcher::dispatch(TransportEnvelope& msg) {
    if (channel_ == nullptr || !channel_->subscribe(msg)) {
        ++stats_.empty_polls;
        return false;
    }
    ++stats_.received;
    return true;
}

TransportEndpointStats ShmDispatcher::stats() const {
    return stats_;
}

Receiver::Receiver(TransportRole role, std::unique_ptr<Dispatcher> dispatcher)
    : role_(std::move(role)), dispatcher_(std::move(dispatcher)) {}

TransportEndpointStats Receiver::stats() const {
    auto out = stats_;
    if (dispatcher_) {
        const auto d = dispatcher_->stats();
        // Dispatcher statistics are diagnostic. Do not add d.received to
        // out.received, otherwise one logical receive is counted twice.
        out.empty_polls += d.empty_polls;
        out.dropped += d.dropped;
        out.size_errors += d.size_errors;
        out.payload_errors += d.payload_errors;
        out.lost += d.lost;
    }
    return out;
}

void Receiver::update_receive_stats(const TransportEnvelope& msg) {
    ++stats_.received;
    if (msg.seq != expected_seq_) {
        stats_.lost += (msg.seq > expected_seq_) ? (msg.seq - expected_seq_) : 1U;
    }
    expected_seq_ = msg.seq + 1U;
    const auto validation = validate_transport_message(msg, role_.qos);
    stats_.size_errors += validation.size_errors;
    stats_.payload_errors += validation.payload_errors;
}

RtpsReceiver::RtpsReceiver(TransportRole role, std::unique_ptr<RtpsDispatcher> dispatcher)
    : Receiver(std::move(role), std::move(dispatcher)) {}

bool RtpsReceiver::receive(TransportEnvelope& msg) {
    if (!dispatcher_ || !dispatcher_->dispatch(msg)) {
        ++stats_.empty_polls;
        return false;
    }
    update_receive_stats(msg);
    return true;
}

ShmReceiver::ShmReceiver(TransportRole role, std::unique_ptr<ShmDispatcher> dispatcher)
    : Receiver(std::move(role), std::move(dispatcher)) {}

bool ShmReceiver::receive(TransportEnvelope& msg) {
    if (!dispatcher_ || !dispatcher_->dispatch(msg)) {
        ++stats_.empty_polls;
        return false;
    }
    update_receive_stats(msg);
    return true;
}

std::shared_ptr<LocalPubSubTransport> Transport::getOrCreateChannel(const TransportRole& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string key = std::string(transport_backend_to_string(role.backend)) + ":" + role.channel_name;
    auto it = channels_.find(key);
    if (it != channels_.end()) {
        return it->second;
    }
    auto channel = std::make_shared<LocalPubSubTransport>(role.channel_name, role.qos);
    channels_[key] = channel;
    return channel;
}

std::unique_ptr<Transmitter> Transport::createTransmitter(const TransportRole& role) {
    auto channel = getOrCreateChannel(role);
    if (role.backend == TransportBackend::RTPS) {
        return std::make_unique<RtpsTransmitter>(role, channel);
    }
    return std::make_unique<ShmTransmitter>(role, channel);
}

std::unique_ptr<Receiver> Transport::createReceiver(const TransportRole& role) {
    auto channel = getOrCreateChannel(role);
    if (role.backend == TransportBackend::RTPS) {
        auto dispatcher = std::make_unique<RtpsDispatcher>(role, channel);
        return std::make_unique<RtpsReceiver>(role, std::move(dispatcher));
    }
    auto dispatcher = std::make_unique<ShmDispatcher>(role, channel);
    return std::make_unique<ShmReceiver>(role, std::move(dispatcher));
}

TransportEndpointStats merge_endpoint_stats(const TransportEndpointStats& a,
                                            const TransportEndpointStats& b) {
    TransportEndpointStats out;
    out.sent = a.sent + b.sent;
    out.received = a.received + b.received;
    out.dropped = a.dropped + b.dropped;
    out.lost = a.lost + b.lost;
    out.size_errors = a.size_errors + b.size_errors;
    out.payload_errors = a.payload_errors + b.payload_errors;
    out.empty_polls = a.empty_polls + b.empty_polls;
    return out;
}

}  // namespace avm
