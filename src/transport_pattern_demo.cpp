/**
 * @file transport_pattern_demo.cpp
 * @brief End-to-end validation for Transport -> Transmitter/Dispatcher/Receiver pattern.
 */
#include "transport_endpoint.hpp"
#include "transport_message.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {
std::string arg_value(int argc, char** argv, const std::string& key, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return default_value;
}

struct Scenario {
    std::string name;
    avm::TransportMessageKind kind;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = 0;
    std::uint32_t point_count = 0;
    std::uint32_t point_stride = 0;
    double ratio = 0.25;
};

std::vector<Scenario> scenarios_for_mode(const std::string& mode) {
    const std::vector<Scenario> all = {
        {"test", avm::TransportMessageKind::TEST, 0, 0, 0, 0, 0, 0.25},
        {"encoded-small", avm::TransportMessageKind::ENCODED_FRAME, 160, 120, 1, 0, 0, 0.25},
        {"lidar-small", avm::TransportMessageKind::LIDAR_FRAME, 0, 0, 0, 1024, 16, 0.25},
        {"raw-small", avm::TransportMessageKind::RAW_FRAME, 160, 120, 1, 0, 0, 0.25},
        {"encoded", avm::TransportMessageKind::ENCODED_FRAME, 640, 480, 1, 0, 0, 0.25},
        {"lidar", avm::TransportMessageKind::LIDAR_FRAME, 0, 0, 0, 10000, 16, 0.25},
        {"raw", avm::TransportMessageKind::RAW_FRAME, 640, 480, 1, 0, 0, 0.25},
    };
    if (mode == "all") {
        return all;
    }
    std::vector<Scenario> out;
    for (const auto& s : all) {
        if (s.name == mode) {
            out.push_back(s);
        }
    }
    return out;
}

avm::TransportEnvelope make_message(const Scenario& s, std::uint64_t seq) {
    switch (s.kind) {
        case avm::TransportMessageKind::TEST:
            return avm::make_test_msg(seq);
        case avm::TransportMessageKind::RAW_FRAME:
            return avm::make_raw_frame(seq, s.width, s.height, s.channels);
        case avm::TransportMessageKind::ENCODED_FRAME:
            return avm::make_encoded_frame(seq, s.width, s.height, s.ratio);
        case avm::TransportMessageKind::LIDAR_FRAME:
            return avm::make_lidar_frame(seq, s.point_count, s.point_stride);
        default:
            return avm::make_test_msg(seq);
    }
}

struct PatternResult {
    std::string backend;
    std::string scenario;
    std::string transmitter_type;
    std::string receiver_type;
    std::uint64_t frames = 0;
    std::uint64_t sent = 0;
    std::uint64_t received = 0;
    std::uint64_t dropped = 0;
    std::uint64_t lost = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::size_t payload_bytes = 0;
    double mean_latency_us = 0.0;
    bool pass = false;
};

PatternResult run_case(avm::Transport& transport, avm::TransportBackend backend,
                       const Scenario& scenario, std::uint64_t frames, std::size_t depth) {
    avm::TransportRole role;
    role.channel_name = "avm/v2_2/" + std::string(avm::transport_backend_to_string(backend)) + "/" + scenario.name;
    role.backend = backend;
    role.qos.msg_size = std::max<std::size_t>(512U * 1024U, make_message(scenario, 0).payload.size() + 64U);
    role.qos.depth = depth;
    role.qos.drop_oldest = true;

    auto transmitter = transport.createTransmitter(role);
    auto receiver = transport.createReceiver(role);

    PatternResult result;
    result.backend = avm::transport_backend_to_string(backend);
    result.scenario = scenario.name;
    result.transmitter_type = transmitter->type_name();
    result.receiver_type = receiver->type_name();
    result.frames = frames;
    result.payload_bytes = make_message(scenario, 0).payload.size();

    std::vector<double> latencies;
    for (std::uint64_t seq = 0; seq < frames; ++seq) {
        auto tx_msg = make_message(scenario, seq);
        std::string error;
        if (!transmitter->transmit(tx_msg, &error)) {
            std::cerr << "[transport_pattern_demo] transmit failed backend=" << result.backend
                      << " scenario=" << scenario.name << " seq=" << seq
                      << " error=" << error << "\n";
        }
        avm::TransportEnvelope rx_msg;
        while (receiver->receive(rx_msg)) {
            latencies.push_back(static_cast<double>(avm::now_us() - rx_msg.timestamp_us));
        }
    }

    const auto tx_stats = transmitter->stats();
    const auto rx_stats = receiver->stats();
    result.sent = tx_stats.sent;
    result.received = rx_stats.received;
    result.dropped = tx_stats.dropped;
    result.lost = rx_stats.lost;
    result.size_errors = tx_stats.size_errors + rx_stats.size_errors;
    result.payload_errors = tx_stats.payload_errors + rx_stats.payload_errors;
    if (!latencies.empty()) {
        result.mean_latency_us = std::accumulate(latencies.begin(), latencies.end(), 0.0) /
                                 static_cast<double>(latencies.size());
    }
    result.pass = result.sent == frames && result.received == frames && result.dropped == 0U &&
                  result.lost == 0U && result.size_errors == 0U && result.payload_errors == 0U;
    return result;
}

void write_csv(const std::string& path, const std::vector<PatternResult>& results) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path);
    out << "backend,scenario,transmitter,receiver,frames,payload_bytes,sent,received,dropped,lost,size_errors,payload_errors,mean_latency_us,pass\n";
    for (const auto& r : results) {
        out << r.backend << ',' << r.scenario << ',' << r.transmitter_type << ',' << r.receiver_type << ','
            << r.frames << ',' << r.payload_bytes << ',' << r.sent << ',' << r.received << ','
            << r.dropped << ',' << r.lost << ',' << r.size_errors << ',' << r.payload_errors << ','
            << std::fixed << std::setprecision(3) << r.mean_latency_us << ','
            << (r.pass ? "PASS" : "FAIL") << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string backend_arg = arg_value(argc, argv, "--backend", "both");
    const std::string mode = arg_value(argc, argv, "--mode", "all");
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "20")));
    const auto depth = static_cast<std::size_t>(std::stoull(arg_value(argc, argv, "--depth", "8")));
    const std::string output = arg_value(argc, argv, "--output", "logs/transport_pattern.csv");

    const auto scenarios = scenarios_for_mode(mode);
    if (scenarios.empty()) {
        std::cerr << "unknown mode=" << mode << "\n";
        return 2;
    }

    std::vector<avm::TransportBackend> backends;
    if (backend_arg == "both") {
        backends = {avm::TransportBackend::RTPS, avm::TransportBackend::SHM};
    } else {
        backends = {avm::parse_transport_backend(backend_arg, avm::TransportBackend::SHM)};
    }

    avm::Transport transport;
    std::vector<PatternResult> results;
    bool all_pass = true;
    for (const auto backend : backends) {
        for (const auto& scenario : scenarios) {
            auto result = run_case(transport, backend, scenario, frames, depth);
            all_pass = all_pass && result.pass;
            std::cout << "[transport_pattern_demo] backend=" << result.backend
                      << " scenario=" << result.scenario
                      << " tx=" << result.transmitter_type
                      << " rx=" << result.receiver_type
                      << " frames=" << result.frames
                      << " payload_bytes=" << result.payload_bytes
                      << " sent=" << result.sent
                      << " received=" << result.received
                      << " dropped=" << result.dropped
                      << " lost=" << result.lost
                      << " size_error=" << result.size_errors
                      << " payload_error=" << result.payload_errors
                      << " mean_us=" << std::fixed << std::setprecision(3) << result.mean_latency_us
                      << " result=" << (result.pass ? "PASS" : "FAIL") << "\n";
            results.push_back(result);
        }
    }
    write_csv(output, results);
    std::cout << "[transport_pattern_demo] csv=" << output << "\n";
    return all_pass ? 0 : 1;
}

