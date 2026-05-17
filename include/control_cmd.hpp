/**
 * @file control_cmd.hpp
 * @brief 控制面命令解析：将字符串命令转换为枚举命令。
 */
#pragma once

#include <algorithm>
#include <string>

enum class ControlCmd {
    START_CAMERA,
    STOP_CAMERA,
    SET_FPS,
    QUERY_STATUS,
    QUERY_FRAME_COUNT,
    QUERY_ERROR_CODE,
    UNKNOWN
};

inline ControlCmd parse_control_cmd(std::string text) {
    const auto pos = text.find(' ');
    std::string head = (pos == std::string::npos) ? text : text.substr(0, pos);
    head.erase(std::remove(head.begin(), head.end(), '\n'), head.end());
    head.erase(std::remove(head.begin(), head.end(), '\r'), head.end());

    if (head == "START_CAMERA") {
        return ControlCmd::START_CAMERA;
    }
    if (head == "STOP_CAMERA") {
        return ControlCmd::STOP_CAMERA;
    }
    if (head == "SET_FPS") {
        return ControlCmd::SET_FPS;
    }
    if (head == "QUERY_STATUS") {
        return ControlCmd::QUERY_STATUS;
    }
    if (head == "QUERY_FRAME_COUNT") {
        return ControlCmd::QUERY_FRAME_COUNT;
    }
    if (head == "QUERY_ERROR_CODE") {
        return ControlCmd::QUERY_ERROR_CODE;
    }
    return ControlCmd::UNKNOWN;
}
