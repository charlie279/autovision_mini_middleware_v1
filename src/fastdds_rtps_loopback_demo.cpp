/**
 * @file fastdds_rtps_loopback_demo.cpp
 * @brief Optional FastDDS/RTPS loopback validation for TransportEnvelope payloads.
 */
#include "fastdds_rtps_transport.hpp"
#include "transport_message.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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
}

int main(int argc, char** argv) {
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "5")));
    const auto depth = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--depth", "8")));
    const std::string topic = arg_value(argc, argv, "--topic", "avm/v2_3/fastdds/raw");
    const std::string output = arg_value(argc, argv, "--output", "logs/benchmark_v2_3_fastdds/fastdds_rtps.csv");
    const bool allow_unavailable = has_flag(argc, argv, "--allow-unavailable");

    std::filesystem::create_directories(std::filesystem::path(output).parent_path());
    std::ofstream csv(output);
    csv << "backend,compiled,topic,frames,sent,received,size_errors,payload_errors,status\n";

    if (!avm::fastdds_support_compiled()) {
        std::cout << "[fastdds_rtps_loopback_demo] status=" << avm::fastdds_support_status() << "\n";
        csv << "fastdds,false," << topic << "," << frames << ",0,0,0,0,NOT_COMPILED\n";
        return allow_unavailable ? 0 : 2;
    }

    avm::FastddsRtpsOptions options;
    options.topic = topic;
    options.depth = depth;
    options.max_payload_size = 512U * 1024U;
    options.reliable = false;

    std::string error;
    avm::FastddsRtpsBus bus(options);
    if (!bus.open(&error)) {
        std::cerr << "[fastdds_rtps_loopback_demo] open failed: " << error << "\n";
        csv << "fastdds,true," << topic << "," << frames << ",0,0,0,0,OPEN_FAILED\n";
        return 3;
    }

    std::uint64_t received = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    for (std::uint64_t i = 0; i < frames; ++i) {
        auto msg = avm::make_raw_frame(i, 640, 480, 1);
        if (!bus.publish(msg, &error)) {
            std::cerr << "[fastdds_rtps_loopback_demo] publish failed: " << error << "\n";
            continue;
        }
        avm::TransportEnvelope got;
        bool ok = false;
        for (int retry = 0; retry < 100 && !ok; ++retry) {
            ok = bus.take(got, &error);
            if (!ok) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        if (ok) {
            const auto check = avm::validate_transport_message(got, avm::TransportQosProfile{});
            ++received;
            size_errors += check.size_errors;
            payload_errors += check.payload_errors;
        }
    }
    const auto stats = bus.stats();
    const bool pass = stats.sent == frames && received == frames && size_errors == 0 && payload_errors == 0;
    csv << "fastdds,true," << topic << "," << frames << "," << stats.sent << "," << received << ","
        << size_errors << "," << payload_errors << "," << (pass ? "PASS" : "FAIL") << "\n";
    std::cout << "[fastdds_rtps_loopback_demo] compiled=true sent=" << stats.sent
              << " received=" << received << " size_errors=" << size_errors
              << " payload_errors=" << payload_errors << " result=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
