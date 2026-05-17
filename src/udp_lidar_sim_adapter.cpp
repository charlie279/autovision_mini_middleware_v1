/**
 * @file udp_lidar_sim_adapter.cpp
 * @brief UDP Lidar simulator adapter implementation.
 */
#include "udp_lidar_sim_adapter.hpp"

#include "avm_config.hpp"
#include "time_utils.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

UdpLidarSimAdapter::UdpLidarSimAdapter(std::string bind_ip, std::uint16_t port,
                                       std::size_t max_payload, int timeout_ms)
    : bind_ip_(std::move(bind_ip)), port_(port), max_payload_(max_payload), timeout_ms_(timeout_ms) {
    buffer_.resize(max_payload_);
}

UdpLidarSimAdapter::~UdpLidarSimAdapter() {
    stop();
}

bool UdpLidarSimAdapter::open() {
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        std::cerr << "[UdpLidarSimAdapter] socket failed: " << std::strerror(errno) << "\n";
        return false;
    }

    int reuse = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (::inet_pton(AF_INET, bind_ip_.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "[UdpLidarSimAdapter] invalid bind_ip=" << bind_ip_ << "\n";
        return false;
    }
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "[UdpLidarSimAdapter] bind " << bind_ip_ << ":" << port_
                  << " failed: " << std::strerror(errno) << "\n";
        return false;
    }
    std::cout << "[UdpLidarSimAdapter] bind=" << bind_ip_ << ":" << port_ << "\n";
    return true;
}

bool UdpLidarSimAdapter::start() {
    started_ = fd_ >= 0;
    return started_;
}

bool UdpLidarSimAdapter::readFrame(SensorFrame& frame) {
    if (!started_ || fd_ < 0) {
        return false;
    }
    pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;
    const int rc = ::poll(&pfd, 1, timeout_ms_);
    if (rc <= 0) {
        return false;
    }

    sockaddr_in src{};
    socklen_t src_len = sizeof(src);
    const ssize_t n = ::recvfrom(fd_, buffer_.data(), buffer_.size(), 0,
                                 reinterpret_cast<sockaddr*>(&src), &src_len);
    if (n <= 0) {
        return false;
    }

    frame = SensorFrame{};
    frame.frame_id = next_frame_id_++;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 1;
    frame.width = static_cast<std::uint32_t>(n / sizeof(float));
    frame.height = 1;
    frame.format = avm::kFormatLidarFloat32;
    frame.data_size = static_cast<std::uint32_t>(n);
    frame.stride_bytes = static_cast<std::uint32_t>(n);
    frame.data.assign(buffer_.begin(), buffer_.begin() + n);
    return true;
}

void UdpLidarSimAdapter::stop() {
    started_ = false;
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

