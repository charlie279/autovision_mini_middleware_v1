/**
 * @file 02_control_cmd.cpp
 * @brief 示例：调用 parse_control_cmd()，理解控制面字符串命令解析。
 */
#include "control_cmd.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {
const char* to_text(ControlCmd cmd) {
    switch (cmd) {
        case ControlCmd::START_CAMERA: return "START_CAMERA";
        case ControlCmd::STOP_CAMERA: return "STOP_CAMERA";
        case ControlCmd::SET_FPS: return "SET_FPS";
        case ControlCmd::QUERY_STATUS: return "QUERY_STATUS";
        case ControlCmd::QUERY_FRAME_COUNT: return "QUERY_FRAME_COUNT";
        case ControlCmd::QUERY_ERROR_CODE: return "QUERY_ERROR_CODE";
        default: return "UNKNOWN";
    }
}
}  // namespace

int main() {
    const std::vector<std::string> commands = {
        "QUERY_STATUS",
        "SET_FPS 15",
        "QUERY_FRAME_COUNT",
        "QUERY_ERROR_CODE",
        "HELLO"
    };

    for (const auto& command : commands) {
        std::cout << "[02_control_cmd] input=\"" << command
                  << "\" parsed=" << to_text(parse_control_cmd(command)) << "\n";
    }
    return 0;
}
