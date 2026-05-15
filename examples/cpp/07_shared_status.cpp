/**
 * @file 07_shared_status.cpp
 * @brief 示例：调用 SharedStatus，理解节点 heartbeat、帧计数和 Safety 状态如何共享。
 */
#include "shared_status.hpp"

#include <iostream>
#include <string>
#include <sys/mman.h>

int main() {
    const std::string name = "/avm_example_status";
    shm_unlink(name.c_str());

    SharedStatus status;
    if (!status.create_or_open(name)) {
        std::cerr << "[07_shared_status] create_or_open failed\n";
        return 1;
    }

    status.update_node("media", 10, 10);
    status.update_node("preprocess", 9, 9, ErrorCode::OK, 0, 0);
    status.update_node("npu", 8, 8);
    status.set_desired_fps(15);
    status.set_safety(SafetyState::NORMAL, ErrorCode::OK, "example normal");

    RuntimeStatusBlock snapshot = status.snapshot();
    std::cout << "[07_shared_status] fps=" << snapshot.desired_fps
              << " media=" << snapshot.media.frame_count
              << " preprocess=" << snapshot.preprocess.frame_count
              << " npu=" << snapshot.npu.frame_count
              << " state=" << safety_state_to_string(static_cast<SafetyState>(snapshot.safety_state))
              << " text=\"" << snapshot.safety_text << "\"\n";

    status.close();
    shm_unlink(name.c_str());
    return 0;
}