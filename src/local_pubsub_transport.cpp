/**
 * @file local_pubsub_transport.cpp
 * @brief Dependency-free bounded queue transport backend.
 */
#include "local_pubsub_transport.hpp"

#include <sstream>

namespace avm {

LocalPubSubTransport::LocalPubSubTransport(std::string channel_name, TransportQosProfile qos)
    : channel_name_(std::move(channel_name)), qos_(qos) {}

bool LocalPubSubTransport::publish(const TransportEnvelope& msg, std::string* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto check = validate_transport_message(msg, qos_);
    if (!check.ok) {
        stats_.size_errors += check.size_errors;
        stats_.payload_errors += check.payload_errors;
        if (error != nullptr) {
            *error = check.reason;
        }
        return false;
    }

    if (qos_.depth == 0U) {
        ++stats_.dropped;
        if (error != nullptr) {
            *error = "qos.depth is zero";
        }
        return false;
    }

    if (queue_.size() >= qos_.depth) {
        if (!qos_.drop_oldest) {
            ++stats_.dropped;
            if (error != nullptr) {
                *error = "queue full";
            }
            return false;
        }
        queue_.pop_front();
        ++stats_.dropped;
    }

    queue_.push_back(msg);
    ++stats_.published;
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool LocalPubSubTransport::subscribe(TransportEnvelope& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    msg = std::move(queue_.front());
    queue_.pop_front();
    ++stats_.received;
    return true;
}

LocalPubSubStats LocalPubSubTransport::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

std::size_t LocalPubSubTransport::depth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

}  // namespace avm
