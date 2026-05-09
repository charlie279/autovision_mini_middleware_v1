/**
 * @file control_service.cpp
 * @brief 控制面服务端：通过 Unix Domain Socket 接收 QUERY_STATUS、SET_FPS 等控制命令。
 */
#include "avm_config.hpp"
#include "control_cmd.hpp"
#include "shared_status.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
std::string handle_command(const std::string& command, SharedStatus& status) {
    RuntimeStatusBlock snapshot = status.snapshot();
    const ControlCmd parsed = parse_control_cmd(command);

    switch (parsed) {
        case ControlCmd::QUERY_STATUS: {
            std::ostringstream oss;
            oss << "status=" << safety_state_to_string(static_cast<SafetyState>(snapshot.safety_state))
                << " fps=" << snapshot.desired_fps
                << " media_frames=" << snapshot.media.frame_count
                << " preprocess_frames=" << snapshot.preprocess.frame_count
                << " npu_frames=" << snapshot.npu.frame_count
                << " error_code=" << snapshot.global_error_code
                << " text=\"" << snapshot.safety_text << "\"\n";
            return oss.str();
        }
        case ControlCmd::QUERY_FRAME_COUNT:
            return "media=" + std::to_string(snapshot.media.frame_count) +
                   " preprocess=" + std::to_string(snapshot.preprocess.frame_count) +
                   " npu=" + std::to_string(snapshot.npu.frame_count) + "\n";
        case ControlCmd::QUERY_ERROR_CODE:
            return "error_code=" + std::to_string(snapshot.global_error_code) +
                   " text=\"" + std::string(snapshot.safety_text) + "\"\n";
        case ControlCmd::SET_FPS: {
            std::istringstream iss(command);
            std::string head;
            int fps = 30;
            iss >> head >> fps;
            if (fps <= 0 || fps > 120) {
                return "ERR invalid fps\n";
            }
            status.set_desired_fps(static_cast<std::uint32_t>(fps));
            return "OK set_fps=" + std::to_string(fps) + "\n";
        }
        case ControlCmd::START_CAMERA:
            return "OK START_CAMERA accepted (V1 stub)\n";
        case ControlCmd::STOP_CAMERA:
            return "OK STOP_CAMERA accepted (V1 stub)\n";
        default:
            return "ERR unknown command\n";
    }
}
}  // namespace

int main() {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    SharedStatus status;
    status.create_or_open(avm::kStatusName);

    ::unlink(avm::kControlSockPath);
    const int server_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[control_service] socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", avm::kControlSockPath);

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "[control_service] bind failed: " << std::strerror(errno) << "\n";
        ::close(server_fd);
        return 2;
    }

    if (::listen(server_fd, 8) != 0) {
        std::cerr << "[control_service] listen failed: " << std::strerror(errno) << "\n";
        ::close(server_fd);
        return 3;
    }

    std::cout << "[control_service] listening on " << avm::kControlSockPath << "\n";

    while (true) {
        const int client_fd = ::accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            continue;
        }

        char buffer[512]{};
        const ssize_t n = ::read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            const std::string response = handle_command(std::string(buffer, static_cast<std::size_t>(n)), status);
            ::write(client_fd, response.data(), response.size());
        }
        ::close(client_fd);
    }
}
