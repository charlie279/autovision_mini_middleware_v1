/**
 * @file camera_fastdds_sub.cpp
 * @brief V2.4 two-process subscriber: FastDDS/RTPS raw camera frames -> validation/raw dump.
 */
#include "fastdds_rtps_transport.hpp"
#include "transport_message.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
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

std::uint32_t bytes_per_pixel(const std::string& format) {
    if (format == "rgb" || format == "rgb888") {
        return 3;
    }
    return 2;
}

std::string normalized_format(const std::string& format) {
    if (format == "rgb" || format == "rgb888") {
        return "rgb";
    }
    return "yuyv";
}

std::uint64_t now_us_local() {
    using Clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count());
}

double percentile(std::vector<double> values, double p) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double rank = (p / 100.0) * static_cast<double>(values.size() - 1U);
    const auto idx = static_cast<std::size_t>(rank);
    return values[std::min(idx, values.size() - 1U)];
}

std::string raw_filename(const std::string& dir, std::uint64_t seq, const std::string& format) {
    std::ostringstream oss;
    oss << dir << "/frame_" << std::setw(6) << std::setfill('0') << seq << "." << format;
    return oss.str();
}

void write_sub_csv_header(std::ofstream& csv) {
    csv << "role,compiled,topic,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status\n";
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string topic = arg_value(argc, argv, "--topic", "avm/camera/raw");
    const std::string format = normalized_format(arg_value(argc, argv, "--format", "yuyv"));
    const auto width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", "640")));
    const auto height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", "480")));
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "300")));
    const auto start_seq = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--start-seq", "1")));
    const auto depth = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--depth", "512")));
    const auto domain_id = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--domain", "0")));
    const auto max_payload = static_cast<std::size_t>(std::stoull(arg_value(argc, argv, "--max-payload", "2097152")));
    const auto timeout_ms = static_cast<int>(std::stoi(arg_value(argc, argv, "--timeout-ms", "60000")));
    const std::string output = arg_value(argc, argv, "--output", "logs/benchmark_v2_4_fastdds_camera/camera_fastdds_sub.csv");
    const std::string save_dir = arg_value(argc, argv, "--save-dir", "logs/benchmark_v2_4_fastdds_camera/frames");
    const auto save_every = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--save-every", "0")));
    const bool reliable = has_flag(argc, argv, "--reliable");
    const bool allow_unavailable = has_flag(argc, argv, "--allow-unavailable");

    std::filesystem::create_directories(std::filesystem::path(output).parent_path());
    if (save_every > 0) {
        std::filesystem::create_directories(save_dir);
    }
    std::ofstream csv(output);
    write_sub_csv_header(csv);

    if (!avm::fastdds_support_compiled()) {
        std::cout << "[camera_fastdds_sub] status=" << avm::fastdds_support_status() << "\n";
        csv << "sub,false," << topic << "," << format << "," << width << "," << height
            << "," << frames << "," << start_seq << ",0,0,0,0,0," << timeout_ms << ",0,0,NOT_COMPILED\n";
        return allow_unavailable ? 0 : 2;
    }

    avm::FastddsRtpsOptions options;
    options.topic = topic;
    options.domain_id = domain_id;
    options.depth = depth;
    options.max_payload_size = max_payload;
    options.reliable = reliable;
    options.participant_name = "autovision_camera_fastdds_sub";

    std::string error;
    avm::FastddsRtpsBus bus(options);
    if (!bus.open(&error)) {
        std::cerr << "[camera_fastdds_sub] open failed: " << error << "\n";
        csv << "sub,true," << topic << "," << format << "," << width << "," << height
            << "," << frames << "," << start_seq << ",0,0,0,0,0," << timeout_ms << ",0,0,OPEN_FAILED\n";
        return 3;
    }

    const std::size_t expected_payload = static_cast<std::size_t>(width) * height * bytes_per_pixel(format);
    const std::uint64_t final_seq = start_seq + frames - 1U;
    std::uint64_t duplicates = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::set<std::uint64_t> seen;
    std::vector<double> latencies_ms;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (seen.size() < frames && std::chrono::steady_clock::now() < deadline) {
        avm::TransportEnvelope msg;
        if (!bus.take(msg, &error)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        if (msg.seq < start_seq || msg.seq > final_seq) {
            continue;
        }
        if (!seen.insert(msg.seq).second) {
            ++duplicates;
            continue;
        }
        const auto check = avm::validate_transport_message(msg, avm::TransportQosProfile{max_payload, depth, true, reliable});
        size_errors += check.size_errors;
        payload_errors += check.payload_errors;
        if (msg.kind != avm::TransportMessageKind::RAW_FRAME || msg.width != width || msg.height != height ||
            msg.payload.size() != expected_payload) {
            ++size_errors;
        }
        if (msg.timestamp_us > 0) {
            const std::uint64_t now = now_us_local();
            if (now >= msg.timestamp_us) {
                latencies_ms.push_back(static_cast<double>(now - msg.timestamp_us) / 1000.0);
            }
        }
        const std::uint64_t received = static_cast<std::uint64_t>(seen.size());
        if (save_every > 0 && (received == 1 || received % save_every == 0)) {
            std::ofstream raw(raw_filename(save_dir, msg.seq, format), std::ios::binary);
            raw.write(reinterpret_cast<const char*>(msg.payload.data()), static_cast<std::streamsize>(msg.payload.size()));
        }
        if (received == 1 || received % 30 == 0 || received == frames) {
            std::cout << "[camera_fastdds_sub] seq=" << msg.seq
                      << " topic=" << topic
                      << " format=" << format
                      << " payload=" << msg.payload.size()
                      << " unique_received=" << received
                      << " duplicates=" << duplicates << "\n";
        }
    }

    std::uint64_t lost = 0;
    for (std::uint64_t seq = start_seq; seq <= final_seq; ++seq) {
        if (seen.find(seq) == seen.end()) {
            ++lost;
        }
    }

    double mean_latency = 0.0;
    for (const double v : latencies_ms) {
        mean_latency += v;
    }
    if (!latencies_ms.empty()) {
        mean_latency /= static_cast<double>(latencies_ms.size());
    }
    const double p95_latency = percentile(latencies_ms, 95.0);
    const auto unique_received = static_cast<std::uint64_t>(seen.size());
    const bool pass = (unique_received == frames && lost == 0 && size_errors == 0 && payload_errors == 0);
    csv << "sub,true," << topic << "," << format << "," << width << "," << height
        << "," << frames << "," << start_seq << "," << unique_received << "," << duplicates << "," << lost
        << "," << size_errors << "," << payload_errors
        << "," << timeout_ms << "," << mean_latency << "," << p95_latency << ","
        << (pass ? "PASS" : "PARTIAL") << "\n";
    std::cout << "[camera_fastdds_sub] done unique_received=" << unique_received
              << " duplicates=" << duplicates
              << " lost=" << lost
              << " size_errors=" << size_errors
              << " payload_errors=" << payload_errors
              << " mean_latency_ms=" << mean_latency
              << " p95_latency_ms=" << p95_latency
              << " result=" << (pass ? "PASS" : "PARTIAL") << "\n";
    return pass ? 0 : 1;
}
