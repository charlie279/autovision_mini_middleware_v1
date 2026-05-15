/**
 * @file control_client.cpp
 * @brief 控制面客户端：向 control_service 发送查询或控制命令并打印响应。
 */
#include "avm_config.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    if (argc < 2) {
        std::cerr << "usage: control_client <COMMAND> [ARG]\n";
        return 1;
    }

    std::ostringstream oss;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            oss << ' ';
        }
        oss << argv[i];
    }
    const std::string command = oss.str();

    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "[control_client] socket failed: " << std::strerror(errno) << "\n";
        return 2;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", avm::kControlSockPath);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "[control_client] connect failed: " << std::strerror(errno) << "\n";
        ::close(fd);
        return 3;
    }

    ::write(fd, command.data(), command.size());

    char buffer[1024]{};
    const ssize_t n = ::read(fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        std::cout << std::string(buffer, static_cast<std::size_t>(n));
    }

    ::close(fd);
    return 0;
}