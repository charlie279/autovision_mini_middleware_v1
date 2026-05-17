#include "timestamp_sync.hpp"
#include "udp_lidar_sim_adapter.hpp"
#include "time_utils.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int main() {
    constexpr std::uint16_t port = 32368;
    UdpLidarSimAdapter adapter("127.0.0.1", port, 256, 1000);
    if (!adapter.open() || !adapter.start()) {
        return 1;
    }

    std::thread sender([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_port = htons(port);
        ::inet_pton(AF_INET, "127.0.0.1", &dst.sin_addr);
        float points[8] = {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};
        ::sendto(fd, points, sizeof(points), 0, reinterpret_cast<sockaddr*>(&dst), sizeof(dst));
        ::close(fd);
    });

    SensorFrame lidar;
    if (!adapter.readFrame(lidar)) {
        sender.join();
        return 2;
    }
    sender.join();

    FrameMeta cam{};
    cam.frame_id = 10;
    cam.sensor_type = 0;
    cam.timestamp_ns = lidar.timestamp_ns + 1'000'000ULL;
    FrameMeta li{};
    li.frame_id = lidar.frame_id;
    li.sensor_type = 1;
    li.timestamp_ns = lidar.timestamp_ns;

    avm::TimestampSync sync(5'000'000ULL);
    sync.push_camera(cam);
    sync.push_lidar(li);
    auto pair = sync.try_sync();
    if (!pair) {
        std::cerr << "sync failed\n";
        return 3;
    }
    std::cout << "[16_udp_lidar_timestamp_sync] lidar_bytes=" << lidar.data_size
              << " points=" << (lidar.data_size / sizeof(float))
              << " synced_delta_ns=" << pair->delta_ns << "\n";
    return 0;
}

