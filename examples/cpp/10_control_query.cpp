/**
 * @file 10_control_query.cpp
 * @brief 示例：用 C++ 直接通过 Unix Domain Socket 查询 control_service 状态。
 */
#include "avm_config.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
    const std::string command = "QUERY_STATUS";

    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "[10_control_query] socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", avm::kControlSockPath);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "[10_control_query] connect failed: " << std::strerror(errno) << "\n";
        std::cerr << "[10_control_query] start service first: cd .. && ./build/control_service\n";
        ::close(fd);
        return 2;
    }

    const ssize_t written = ::write(fd, command.data(), command.size());
    if (written < 0) {
        std::cerr << "[10_control_query] write failed: " << std::strerror(errno) << "\n";
        ::close(fd);
        return 3;
    }

    char buffer[1024]{};
    const ssize_t n = ::read(fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        std::cout << "[10_control_query] response: "
                  << std::string(buffer, static_cast<std::size_t>(n));
    }

    ::close(fd);
    return 0;
}