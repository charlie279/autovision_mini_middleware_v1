/**
 * @file camera_dds_shm_sub.cpp
 * @brief V2.5 subscriber: FastDDS/RTPS descriptor metadata -> POSIX SHM raw payload validation.
 */
#include "crc32.hpp"
#include "dds_shm_frame_meta.hpp"
#include "fastdds_rtps_transport.hpp"
#include "shm_frame_pool.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
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

void ensure_parent_dir(const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
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
    csv << "role,compiled,topic,shm_name,format,width,height,frames_expected,start_seq,unique_received,duplicates,lost,metadata_errors,shm_read_errors,size_errors,payload_errors,timeout_ms,mean_latency_ms,p95_latency_ms,status\n";
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string topic = arg_value(argc, argv, "--topic", "avm/camera/dds_shm_meta");
    const std::string expected_shm_name = arg_value(argc, argv, "--shm-name", "/avm_camera_dds_shm_pool");
    const std::string format = avm::dds_shm_normalized_format(arg_value(argc, argv, "--format", "yuyv"));
    const auto width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", "640")));
    const auto height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", "480")));
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "300")));
    const auto start_seq = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--start-seq", "1")));
    const auto depth = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--depth", "1024")));
    const auto domain_id = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--domain", "0")));
    const auto max_payload = static_cast<std::size_t>(std::stoull(arg_value(argc, argv, "--max-payload", "8192")));
    const auto timeout_ms = static_cast<int>(std::stoi(arg_value(argc, argv, "--timeout-ms", "120000")));
    const std::string output = arg_value(argc, argv, "--output", "logs/benchmark_v2_5_dds_shm_synthetic/camera_dds_shm_sub.csv");
    const std::string save_dir = arg_value(argc, argv, "--save-dir", "logs/benchmark_v2_5_dds_shm_synthetic/frames");
    const auto save_every = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--save-every", "0")));
    const bool reliable = has_flag(argc, argv, "--reliable");
    const bool allow_unavailable = has_flag(argc, argv, "--allow-unavailable");

    ensure_parent_dir(output);
    if (save_every > 0) {
        std::filesystem::create_directories(save_dir);
    }
    std::ofstream csv(output);
    write_sub_csv_header(csv);

    if (!avm::fastdds_support_compiled()) {
        std::cout << "[camera_dds_shm_sub] status=" << avm::fastdds_support_status() << "\n";
        csv << "sub,false," << topic << "," << expected_shm_name << "," << format << "," << width << "," << height
            << "," << frames << "," << start_seq << ",0,0,0,0,0,0,0," << timeout_ms << ",0,0,NOT_COMPILED\n";
        return allow_unavailable ? 0 : 2;
    }

    avm::FastddsRtpsOptions options;
    options.topic = topic;
    options.domain_id = domain_id;
    options.depth = depth;
    options.max_payload_size = max_payload;
    options.reliable = reliable;
    options.participant_name = "autovision_camera_dds_shm_sub";

    std::string error;
    avm::FastddsRtpsBus bus(options);
    if (!bus.open(&error)) {
        std::cerr << "[camera_dds_shm_sub] open FastDDS failed: " << error << "\n";
        csv << "sub,true," << topic << "," << expected_shm_name << "," << format << "," << width << "," << height
            << "," << frames << "," << start_seq << ",0,0,0,1,0,0,0," << timeout_ms << ",0,0,OPEN_FAILED\n";
        return 3;
    }

    ShmFramePool pool;
    bool pool_opened = false;
    std::string current_shm_name;
    std::uint32_t current_buffer_count = 0;
    std::uint64_t current_buffer_size = 0;

    const std::uint64_t final_seq = start_seq + frames - 1U;
    std::uint64_t duplicates = 0;
    std::uint64_t metadata_errors = 0;
    std::uint64_t shm_read_errors = 0;
    std::uint64_t size_errors = 0;
    std::uint64_t payload_errors = 0;
    std::set<std::uint64_t> seen;
    std::vector<double> latencies_ms;
    std::vector<std::uint8_t> scratch;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (seen.size() < frames && std::chrono::steady_clock::now() < deadline) {
        avm::TransportEnvelope msg;
        if (!bus.take(msg, &error)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        avm::SharedFrameDescriptor desc;
        if (!avm::parse_shared_frame_descriptor_envelope(msg, desc, &error)) {
            ++metadata_errors;
            std::cerr << "[camera_dds_shm_sub] descriptor parse failed: " << error << "\n";
            continue;
        }
        if (desc.seq < start_seq || desc.seq > final_seq) {
            continue;
        }
        if (!seen.insert(desc.seq).second) {
            ++duplicates;
            continue;
        }

        if (desc.width != width || desc.height != height || avm::dds_shm_format_to_string(desc.format) != format) {
            ++size_errors;
            std::cerr << "[camera_dds_shm_sub] descriptor format/size mismatch seq=" << desc.seq << "\n";
        }
        if (expected_shm_name != desc.shm_name) {
            ++metadata_errors;
            std::cerr << "[camera_dds_shm_sub] shm_name mismatch expected=" << expected_shm_name
                      << " actual=" << desc.shm_name << "\n";
            continue;
        }
        if (!pool_opened || current_shm_name != desc.shm_name ||
            current_buffer_count != desc.buffer_count || current_buffer_size != desc.buffer_size) {
            pool.close();
            if (!pool.open(desc.shm_name, desc.buffer_count, static_cast<std::size_t>(desc.buffer_size))) {
                ++shm_read_errors;
                std::cerr << "[camera_dds_shm_sub] open SHM pool failed name=" << desc.shm_name << "\n";
                continue;
            }
            pool_opened = true;
            current_shm_name = desc.shm_name;
            current_buffer_count = desc.buffer_count;
            current_buffer_size = desc.buffer_size;
        }

        if (desc.payload_size > desc.buffer_size || desc.buffer_index >= desc.buffer_count) {
            ++size_errors;
            continue;
        }
        scratch.resize(static_cast<std::size_t>(desc.payload_size));
        if (!pool.read(desc.buffer_index, scratch.data(), scratch.size())) {
            ++shm_read_errors;
            continue;
        }
        const auto crc = crc32_compute(scratch.data(), scratch.size());
        if (crc != desc.payload_crc32) {
            ++payload_errors;
            std::cerr << "[camera_dds_shm_sub] payload CRC mismatch seq=" << desc.seq << "\n";
        }
        const auto expected_payload = static_cast<std::size_t>(width) * height * avm::dds_shm_bytes_per_pixel(format);
        if (scratch.size() != expected_payload) {
            ++size_errors;
        }
        if (desc.timestamp_us > 0) {
            const std::uint64_t now = now_us_local();
            if (now >= desc.timestamp_us) {
                latencies_ms.push_back(static_cast<double>(now - desc.timestamp_us) / 1000.0);
            }
        }

        const std::uint64_t received = static_cast<std::uint64_t>(seen.size());
        if (save_every > 0 && (received == 1 || received % save_every == 0)) {
            std::ofstream raw(raw_filename(save_dir, desc.seq, format), std::ios::binary);
            raw.write(reinterpret_cast<const char*>(scratch.data()), static_cast<std::streamsize>(scratch.size()));
        }
        if (received == 1 || received % 30 == 0 || received == frames) {
            std::cout << "[camera_dds_shm_sub] seq=" << desc.seq
                      << " slot=" << desc.buffer_index
                      << " metadata_bytes=" << msg.payload.size()
                      << " shm_payload_bytes=" << scratch.size()
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
    const bool pass = (unique_received == frames && lost == 0 && metadata_errors == 0 && shm_read_errors == 0 &&
                       size_errors == 0 && payload_errors == 0);

    csv << "sub,true," << topic << "," << expected_shm_name << "," << format << "," << width << "," << height
        << "," << frames << "," << start_seq << "," << unique_received << "," << duplicates << "," << lost
        << "," << metadata_errors << "," << shm_read_errors << "," << size_errors << "," << payload_errors
        << "," << timeout_ms << "," << mean_latency << "," << p95_latency << ","
        << (pass ? "PASS" : "PARTIAL") << "\n";
    std::cout << "[camera_dds_shm_sub] done unique_received=" << unique_received
              << " duplicates=" << duplicates
              << " lost=" << lost
              << " metadata_errors=" << metadata_errors
              << " shm_read_errors=" << shm_read_errors
              << " size_errors=" << size_errors
              << " payload_errors=" << payload_errors
              << " mean_latency_ms=" << mean_latency
              << " p95_latency_ms=" << p95_latency
              << " result=" << (pass ? "PASS" : "PARTIAL") << "\n";
    return pass ? 0 : 1;
}
