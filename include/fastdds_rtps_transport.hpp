/**
 * @file fastdds_rtps_transport.hpp
 * @brief Optional FastDDS/RTPS adapter for AutoVision transport messages.
 *
 * This header intentionally exposes only AutoVision-owned types. The implementation
 * can be compiled with AVM_HAS_FASTDDS when FastDDS/Fast-RTPS 2.12.x is installed.
 */
#pragma once

#include "transport_message.hpp"

#include <memory>
#include <string>
#include <vector>

namespace avm {

struct FastddsRtpsOptions {
    std::string topic = "avm/fastdds/default";
    std::uint32_t domain_id = 0;
    std::uint32_t depth = 8;
    std::size_t max_payload_size = 2U * 1024U * 1024U;
    bool reliable = false;
    std::string participant_name = "autovision_fastdds_rtps";
    std::string bind_ip = "127.0.0.1";
};

struct FastddsRtpsStats {
    std::uint64_t sent = 0;
    std::uint64_t received = 0;
    std::uint64_t dropped = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::uint64_t empty_polls = 0;
};

bool fastdds_support_compiled();
std::string fastdds_support_status();

std::vector<std::uint8_t> serialize_transport_envelope(const TransportEnvelope& msg);
bool deserialize_transport_envelope(const std::vector<std::uint8_t>& bytes, TransportEnvelope& msg,
                                    std::string* error = nullptr);

class FastddsRtpsBus {
public:
    explicit FastddsRtpsBus(FastddsRtpsOptions options);
    ~FastddsRtpsBus();

    FastddsRtpsBus(const FastddsRtpsBus&) = delete;
    FastddsRtpsBus& operator=(const FastddsRtpsBus&) = delete;

    bool open(std::string* error = nullptr);
    bool publish(const TransportEnvelope& msg, std::string* error = nullptr);
    bool take(TransportEnvelope& msg, std::string* error = nullptr);
    void close();

    FastddsRtpsStats stats() const;
    const FastddsRtpsOptions& options() const { return options_; }

private:
    class Impl;
    FastddsRtpsOptions options_;
    std::unique_ptr<Impl> impl_;
};

}  // namespace avm

