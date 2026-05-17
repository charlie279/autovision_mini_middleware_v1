/**
 * @file algorithm_stub_node.cpp
 * @brief Algorithm-platform stub: converts RuntimeOutput records into DetectionResult schema.
 */
#include "detection_result.hpp"
#include "time_utils.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

namespace {
std::string arg_value(int argc, char** argv, const std::string& key, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) return argv[i + 1];
    }
    return default_value;
}

std::uint64_t parse_u64(const std::string& s, const std::string& key, std::uint64_t def = 0) {
    std::regex re("\\\"" + key + "\\\"\\s*:\\s*([0-9]+)");
    std::smatch m;
    if (std::regex_search(s, m, re)) return static_cast<std::uint64_t>(std::stoull(m[1].str()));
    return def;
}

int parse_i32(const std::string& s, const std::string& key, int def = 0) {
    std::regex re("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch m;
    if (std::regex_search(s, m, re)) return std::stoi(m[1].str());
    return def;
}

double parse_f64(const std::string& s, const std::string& key, double def = 0.0) {
    std::regex re("\\\"" + key + "\\\"\\s*:\\s*([0-9]+(\\.[0-9]+)?)");
    std::smatch m;
    if (std::regex_search(s, m, re)) return std::stod(m[1].str());
    return def;
}

std::string parse_string(const std::string& s, const std::string& key, const std::string& def = "") {
    std::regex re("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch m;
    if (std::regex_search(s, m, re)) return m[1].str();
    return def;
}
}

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    const std::string input = arg_value(argc, argv, "--input", "logs/npu_results.jsonl");
    const std::string output = arg_value(argc, argv, "--output", "logs/detection_results.jsonl");
    std::filesystem::create_directories(std::filesystem::path(output).parent_path());

    std::ifstream in(input);
    if (!in.is_open()) {
        std::cerr << "[algorithm_stub_node] open input failed: " << input << "\n";
        return 1;
    }
    std::ofstream out(output, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "[algorithm_stub_node] open output failed: " << output << "\n";
        return 2;
    }

    std::string line;
    std::uint64_t count = 0;
    while (std::getline(in, line)) {
        RuntimeOutput ro;
        ro.frame_id = parse_u64(line, "frame_id");
        ro.object_count = parse_i32(line, "object_count");
        ro.confidence = parse_f64(line, "confidence");
        ro.latency_ms = parse_f64(line, "npu_latency_ms");
        ro.backend = parse_string(line, "backend", "unknown");
        DetectionResult dr = make_stub_detection_result(ro, avm::now_ns());
        out << dr.to_json() << '\n';
        ++count;
        if (count == 1 || count % 30 == 0) {
            std::cout << "[algorithm_stub_node] frame_id=" << ro.frame_id
                      << " detections=" << dr.boxes.size() << "\n";
        }
    }
    std::cout << "[algorithm_stub_node] finished detections=" << count << " output=" << output << "\n";
    return 0;
}
