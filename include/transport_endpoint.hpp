/**
 * @file transport_endpoint.hpp
 * @brief reference transport-style Transport / Transmitter / Dispatcher / Receiver abstraction for AutoVision V2.2.
 *
 * This module mirrors the communication pattern shown in the design diagram:
 *
 *   publisher -> Transport::createTransmitter()
 *             -> Transmitter base
 *             -> RtpsTransmitter or ShmTransmitter
 *             -> backend channel
 *             -> RtpsDispatcher or ShmDispatcher
 *             -> RtpsReceiver or ShmReceiver
 *             -> Receiver base -> subscriber
 *
 * The current generic Linux implementation intentionally avoids a hard FastDDS dependency.
 * RtpsTransmitter/RtpsReceiver are compile-safe RTPS-style local backends used to validate
 * abstraction boundaries, message size, sequence, CRC and payload integrity. A real FastDDS
 * backend can replace the internal channel implementation later without changing the upper API.
 */
#pragma once

#include "fastdds_rtps_transport.hpp"
#include "local_pubsub_transport.hpp"
#include "transport_message.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace avm {

enum class TransportBackend : std::uint32_t {
    RTPS = 0,
    SHM = 1,
    FASTDDS_RTPS = 2
};

struct TransportRole {
    std::string channel_name = "avm/default";
    TransportBackend backend = TransportBackend::SHM;
    TransportQosProfile qos{};
};

struct TransportEndpointStats {
    std::uint64_t sent = 0;
    std::uint64_t received = 0;
    std::uint64_t dropped = 0;
    std::uint64_t lost = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::uint64_t empty_polls = 0;
};

const char* transport_backend_to_string(TransportBackend backend);
TransportBackend parse_transport_backend(const std::string& text, TransportBackend fallback = TransportBackend::SHM);

class Transmitter {
public:
    explicit Transmitter(TransportRole role);
    virtual ~Transmitter() = default;

    virtual bool transmit(const TransportEnvelope& msg, std::string* error = nullptr) = 0;
    virtual TransportEndpointStats stats() const = 0;
    virtual const char* type_name() const = 0;

    const TransportRole& role() const { return role_; }

protected:
    TransportRole role_;
};

class RtpsTransmitter final : public Transmitter {
public:
    RtpsTransmitter(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel);
    bool transmit(const TransportEnvelope& msg, std::string* error = nullptr) override;
    TransportEndpointStats stats() const override;
    const char* type_name() const override { return "RtpsTransmitter(local_rtps_style)"; }

private:
    std::shared_ptr<LocalPubSubTransport> channel_;
};

class ShmTransmitter final : public Transmitter {
public:
    ShmTransmitter(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel);
    bool transmit(const TransportEnvelope& msg, std::string* error = nullptr) override;
    TransportEndpointStats stats() const override;
    const char* type_name() const override { return "ShmTransmitter(local_shm_style)"; }

private:
    std::shared_ptr<LocalPubSubTransport> channel_;
};

class FastddsRtpsTransmitter final : public Transmitter {
public:
    FastddsRtpsTransmitter(TransportRole role, std::shared_ptr<FastddsRtpsBus> bus);
    bool transmit(const TransportEnvelope& msg, std::string* error = nullptr) override;
    TransportEndpointStats stats() const override;
    const char* type_name() const override { return "FastddsRtpsTransmitter(real_fastdds_optional)"; }

private:
    std::shared_ptr<FastddsRtpsBus> bus_;
};

class Dispatcher {
public:
    explicit Dispatcher(TransportRole role);
    virtual ~Dispatcher() = default;

    virtual bool dispatch(TransportEnvelope& msg) = 0;
    virtual TransportEndpointStats stats() const = 0;
    virtual const char* type_name() const = 0;

protected:
    TransportRole role_;
};

class RtpsDispatcher final : public Dispatcher {
public:
    RtpsDispatcher(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel);
    bool dispatch(TransportEnvelope& msg) override;
    TransportEndpointStats stats() const override;
    const char* type_name() const override { return "RtpsDispatcher(local_rtps_style)"; }

private:
    std::shared_ptr<LocalPubSubTransport> channel_;
    mutable TransportEndpointStats stats_{};
};

class ShmDispatcher final : public Dispatcher {
public:
    ShmDispatcher(TransportRole role, std::shared_ptr<LocalPubSubTransport> channel);
    bool dispatch(TransportEnvelope& msg) override;
    TransportEndpointStats stats() const override;
    const char* type_name() const override { return "ShmDispatcher(local_shm_style)"; }

private:
    std::shared_ptr<LocalPubSubTransport> channel_;
    mutable TransportEndpointStats stats_{};
};

class FastddsRtpsDispatcher final : public Dispatcher {
public:
    FastddsRtpsDispatcher(TransportRole role, std::shared_ptr<FastddsRtpsBus> bus);
    bool dispatch(TransportEnvelope& msg) override;
    TransportEndpointStats stats() const override;
    const char* type_name() const override { return "FastddsRtpsDispatcher(real_fastdds_optional)"; }

private:
    std::shared_ptr<FastddsRtpsBus> bus_;
    mutable TransportEndpointStats stats_{};
};

class Receiver {
public:
    explicit Receiver(TransportRole role, std::unique_ptr<Dispatcher> dispatcher);
    virtual ~Receiver() = default;

    virtual bool receive(TransportEnvelope& msg) = 0;
    virtual TransportEndpointStats stats() const;
    virtual const char* type_name() const = 0;

    const TransportRole& role() const { return role_; }

protected:
    TransportRole role_;
    std::unique_ptr<Dispatcher> dispatcher_;
    mutable TransportEndpointStats stats_{};
    std::uint64_t expected_seq_ = 0;

    void update_receive_stats(const TransportEnvelope& msg);
};

class RtpsReceiver final : public Receiver {
public:
    RtpsReceiver(TransportRole role, std::unique_ptr<RtpsDispatcher> dispatcher);
    bool receive(TransportEnvelope& msg) override;
    const char* type_name() const override { return "RtpsReceiver(local_rtps_style)"; }
};

class ShmReceiver final : public Receiver {
public:
    ShmReceiver(TransportRole role, std::unique_ptr<ShmDispatcher> dispatcher);
    bool receive(TransportEnvelope& msg) override;
    const char* type_name() const override { return "ShmReceiver(local_shm_style)"; }
};

class FastddsRtpsReceiver final : public Receiver {
public:
    FastddsRtpsReceiver(TransportRole role, std::unique_ptr<FastddsRtpsDispatcher> dispatcher);
    bool receive(TransportEnvelope& msg) override;
    const char* type_name() const override { return "FastddsRtpsReceiver(real_fastdds_optional)"; }
};

class Transport {
public:
    std::unique_ptr<Transmitter> createTransmitter(const TransportRole& role);
    std::unique_ptr<Receiver> createReceiver(const TransportRole& role);

private:
    std::shared_ptr<LocalPubSubTransport> getOrCreateChannel(const TransportRole& role);
    std::shared_ptr<FastddsRtpsBus> getOrCreateFastddsChannel(const TransportRole& role);

    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<LocalPubSubTransport>> channels_;
    std::map<std::string, std::shared_ptr<FastddsRtpsBus>> fastdds_channels_;
};

TransportEndpointStats merge_endpoint_stats(const TransportEndpointStats& a,
                                            const TransportEndpointStats& b);

}  // namespace avm

