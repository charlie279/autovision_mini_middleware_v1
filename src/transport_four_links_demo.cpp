/**
 * @file transport_four_links_demo.cpp
 * @brief AutoVision V2.1 reference transport-style four-link large payload validation demo.
 */
#include "local_pubsub_transport.hpp"
#include "transport_message.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
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

bool has_flag(int argc, char** argv, const std::string& key) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == key) {
            return true;
        }
    }
    return false;
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
    std::uint32_t fps = 15;
};

std::vector<Scenario> scenarios_for_mode(const std::string& mode) {
    const std::vector<Scenario> all = {
        {"test", avm::TransportMessageKind::TEST, 0, 0, 0, 0, 0, 0.25, 30},
        {"encoded-small", avm::TransportMessageKind::ENCODED_FRAME, 160, 120, 1, 0, 0, 0.25, 15},
        {"lidar-small", avm::TransportMessageKind::LIDAR_FRAME, 0, 0, 0, 1024, 16, 0.25, 10},
        {"raw-small", avm::TransportMessageKind::RAW_FRAME, 160, 120, 1, 0, 0, 0.25, 15},
        {"encoded", avm::TransportMessageKind::ENCODED_FRAME, 640, 480, 1, 0, 0, 0.25, 15},
        {"lidar", avm::TransportMessageKind::LIDAR_FRAME, 0, 0, 0, 10000, 16, 0.25, 10},
        {"raw", avm::TransportMessageKind::RAW_FRAME, 640, 480, 1, 0, 0, 0.25, 15},
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

std::size_t expected_size_for_scenario(const Scenario& s) {
    auto msg = make_message(s, 0);
    return msg.payload.size();
}

struct ScenarioResult {
    std::string name;
    std::string kind;
    std::uint64_t frames = 0;
    std::uint64_t published = 0;
    std::uint64_t received = 0;
    std::uint64_t dropped = 0;
    std::uint64_t lost = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::size_t payload_bytes = 0;
    double mean_latency_us = 0.0;
    double p95_latency_us = 0.0;
    bool pass = false;
};

double percentile(std::vector<double> v, double p) {
    if (v.empty()) {
        return 0.0;
    }
    std::sort(v.begin(), v.end());
    const double idx = (static_cast<double>(v.size() - 1U) * p);
    const std::size_t lo = static_cast<std::size_t>(idx);
    const std::size_t hi = std::min<std::size_t>(lo + 1U, v.size() - 1U);
    const double frac = idx - static_cast<double>(lo);
    return v[lo] * (1.0 - frac) + v[hi] * frac;
}

ScenarioResult run_scenario(const Scenario& s, std::uint64_t frames,
                            std::size_t qos_depth, std::size_t max_payload,
                            bool pace) {
    avm::TransportQosProfile qos;
    qos.depth = qos_depth;
    qos.drop_oldest = true;
    qos.reliable = false;
    qos.msg_size = std::max(max_payload, expected_size_for_scenario(s) + 64U);

    avm::LocalPubSubTransport bus("avm/cammw/" + s.name, qos);
    ScenarioResult result;
    result.name = s.name;
    result.kind = avm::transport_kind_to_string(s.kind);
    result.frames = frames;
    result.payload_bytes = expected_size_for_scenario(s);
    std::vector<double> latencies;
    std::uint64_t expected_seq = 0;

    for (std::uint64_t seq = 0; seq < frames; ++seq) {
        auto msg = make_message(s, seq);
        std::string error;
        if (!bus.publish(msg, &error)) {
            std::cerr << "[transport_four_links_demo] publish failed scenario=" << s.name
                      << " seq=" << seq << " error=" << error << "\n";
        }

        avm::TransportEnvelope rx;
        while (bus.subscribe(rx)) {
            const auto v = avm::validate_transport_message(rx, qos);
            result.size_errors += v.size_errors;
            result.payload_errors += v.payload_errors;
            if (rx.seq != expected_seq) {
                result.lost += (rx.seq > expected_seq) ? (rx.seq - expected_seq) : 1U;
            }
            expected_seq = rx.seq + 1U;
            latencies.push_back(static_cast<double>(avm::now_us() - rx.timestamp_us));
        }

        if (pace && s.fps > 0U) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000000U / s.fps));
        }
    }

    const auto stats = bus.stats();
    result.published = stats.published;
    result.received = stats.received;
    result.dropped = stats.dropped;
    result.size_errors += stats.size_errors;
    result.payload_errors += stats.payload_errors;
    result.lost += stats.lost;
    if (!latencies.empty()) {
        result.mean_latency_us = std::accumulate(latencies.begin(), latencies.end(), 0.0) /
                                 static_cast<double>(latencies.size());
        result.p95_latency_us = percentile(latencies, 0.95);
    }
    result.pass = result.published == frames && result.received == frames && result.dropped == 0 &&
                  result.lost == 0 && result.size_errors == 0 && result.payload_errors == 0;
    return result;
}

void write_csv(const std::string& path, const std::vector<ScenarioResult>& results) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path);
    out << "scenario,kind,frames,payload_bytes,published,received,dropped,lost,size_errors,payload_errors,mean_latency_us,p95_latency_us,pass\n";
    for (const auto& r : results) {
        out << r.name << ',' << r.kind << ',' << r.frames << ',' << r.payload_bytes << ','
            << r.published << ',' << r.received << ',' << r.dropped << ',' << r.lost << ','
            << r.size_errors << ',' << r.payload_errors << ',' << std::fixed << std::setprecision(3)
            << r.mean_latency_us << ',' << r.p95_latency_us << ',' << (r.pass ? "PASS" : "FAIL") << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = arg_value(argc, argv, "--mode", "all");
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "30")));
    const auto depth = static_cast<std::size_t>(std::stoull(arg_value(argc, argv, "--depth", "8")));
    const auto max_payload = static_cast<std::size_t>(std::stoull(arg_value(argc, argv, "--max-payload", "524288")));
    const std::string output = arg_value(argc, argv, "--output", "logs/transport_four_links.csv");
    const bool pace = has_flag(argc, argv, "--pace");

    const auto scenarios = scenarios_for_mode(mode);
    if (scenarios.empty()) {
        std::cerr << "unknown mode=" << mode
                  << " available: all test encoded-small lidar-small raw-small encoded lidar raw\n";
        return 2;
    }

    std::vector<ScenarioResult> results;
    bool all_pass = true;
    for (const auto& s : scenarios) {
        auto r = run_scenario(s, frames, depth, max_payload, pace);
        all_pass = all_pass && r.pass;
        std::cout << "[transport_four_links_demo] scenario=" << r.name
                  << " kind=" << r.kind
                  << " frames=" << r.frames
                  << " payload_bytes=" << r.payload_bytes
                  << " published=" << r.published
                  << " received=" << r.received
                  << " dropped=" << r.dropped
                  << " lost=" << r.lost
                  << " size_error=" << r.size_errors
                  << " payload_error=" << r.payload_errors
                  << " mean_us=" << std::fixed << std::setprecision(3) << r.mean_latency_us
                  << " p95_us=" << r.p95_latency_us
                  << " result=" << (r.pass ? "PASS" : "FAIL") << "\n";
        results.push_back(r);
    }

    write_csv(output, results);
    std::cout << "[transport_four_links_demo] csv=" << output << "\n";
    return all_pass ? 0 : 1;
}

