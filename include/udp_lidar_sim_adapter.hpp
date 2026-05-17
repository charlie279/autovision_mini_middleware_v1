/**
 * @file udp_lidar_sim_adapter.hpp
 * @brief UDP Lidar simulator adapter. It accepts UDP packets carrying float32 ranges/points.
 */
#pragma once

#include "sensor_adapter.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class UdpLidarSimAdapter final : public SensorAdapter {
public:
    UdpLidarSimAdapter(std::string bind_ip = "0.0.0.0", std::uint16_t port = 2368,
                       std::size_t max_payload = 4096, int timeout_ms = 100);
    ~UdpLidarSimAdapter() override;

    bool open() override;
    bool start() override;
    bool readFrame(SensorFrame& frame) override;
    void stop() override;

private:
    std::string bind_ip_;
    std::uint16_t port_ = 2368;
    std::size_t max_payload_ = 4096;
    int timeout_ms_ = 100;
    int fd_ = -1;
    bool started_ = false;
    std::uint64_t next_frame_id_ = 1;
    std::vector<std::uint8_t> buffer_;
};
