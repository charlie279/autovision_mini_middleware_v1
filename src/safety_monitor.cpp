/**
 * @file safety_monitor.cpp
 * @brief Safety Monitor：周期性读取共享状态，检查 heartbeat、CRC 错误和 frame_id 跳变并更新安全状态机。
 */
#include "avm_config.hpp"
#include "shared_status.hpp"
#include "time_utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

namespace {
std::string arg_value(int argc, char** argv, const std::string& key, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return default_value;
}

bool heartbeat_timeout(const NodeRuntimeStatus& node, std::uint64_t now_ns, std::uint64_t threshold_ns) {
    return node.heartbeat_ns > 0 && now_ns > node.heartbeat_ns && (now_ns - node.heartbeat_ns) > threshold_ns;
}
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const int duration_sec = std::stoi(arg_value(argc, argv, "--duration", "20"));
    const int period_ms = std::stoi(arg_value(argc, argv, "--period-ms", "200"));
    const std::uint64_t timeout_ns =
        static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--timeout-ms", "1500"))) * 1'000'000ULL;

    SharedStatus status;
    status.create_or_open(avm::kStatusName);

    SafetyState current_state = SafetyState::NORMAL;
    std::uint64_t last_crc_errors = 0;
    std::uint64_t last_frame_jumps = 0;
    int fault_score = 0;
    const int ticks = duration_sec * 1000 / period_ms;

    for (int i = 0; i < ticks; ++i) {
        RuntimeStatusBlock snapshot = status.snapshot();
        const std::uint64_t now = avm::now_ns();

        const bool media_timeout = heartbeat_timeout(snapshot.media, now, timeout_ns);
        const bool preprocess_timeout = heartbeat_timeout(snapshot.preprocess, now, timeout_ns);
        const bool npu_timeout = heartbeat_timeout(snapshot.npu, now, timeout_ns);
        const bool crc_increased = snapshot.preprocess.crc_error_count > last_crc_errors;
        const bool jump_increased = snapshot.preprocess.frame_jump_count > last_frame_jumps;

        ErrorCode error_code = ErrorCode::OK;
        std::string reason = "NORMAL";

        if (media_timeout || preprocess_timeout || npu_timeout) {
            error_code = ErrorCode::HEARTBEAT_TIMEOUT;
            reason = "heartbeat timeout";
            fault_score += 2;
        } else if (crc_increased) {
            error_code = ErrorCode::CRC_ERROR;
            reason = "crc error detected";
            fault_score += 1;
        } else if (jump_increased) {
            error_code = ErrorCode::FRAME_ID_JUMP;
            reason = "frame id jump detected";
            fault_score += 1;
        } else {
            fault_score = std::max(0, fault_score - 1);
        }

        SafetyState next_state = SafetyState::NORMAL;
        if (fault_score >= 4) {
            next_state = SafetyState::FAULT;
        } else if (fault_score > 0) {
            next_state = SafetyState::DEGRADED;
        }

        if (next_state != current_state) {
            std::cout << "[Safety] state change: " << safety_state_to_string(current_state)
                      << " -> " << safety_state_to_string(next_state)
                      << " reason=" << reason << "\n";
            current_state = next_state;
        }

        status.set_safety(current_state, error_code, reason);
        last_crc_errors = snapshot.preprocess.crc_error_count;
        last_frame_jumps = snapshot.preprocess.frame_jump_count;

        if (i == 0 || i % 5 == 0) {
            std::cout << "[Safety] state=" << safety_state_to_string(current_state)
                      << " media=" << snapshot.media.frame_count
                      << " preprocess=" << snapshot.preprocess.frame_count
                      << " npu=" << snapshot.npu.frame_count
                      << " crc_errors=" << snapshot.preprocess.crc_error_count
                      << " frame_jumps=" << snapshot.preprocess.frame_jump_count << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
    }

    return 0;
}