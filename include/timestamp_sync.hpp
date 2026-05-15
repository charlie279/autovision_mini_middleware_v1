
/**
 * @file timestamp_sync.hpp
 * @brief Approximate timestamp synchronizer for camera and lidar metadata.
 */
#pragma once

#include "frame_meta.hpp"

#include <cstdint>
#include <deque>
#include <optional>

namespace avm {

struct SyncedFramePair {
    FrameMeta camera;
    FrameMeta lidar;
    std::uint64_t delta_ns = 0;
};

class TimestampSync {
public:
    explicit TimestampSync(std::uint64_t tolerance_ns = 50'000'000ULL, std::size_t max_queue = 32);
    void push_camera(const FrameMeta& meta);
    void push_lidar(const FrameMeta& meta);
    std::optional<SyncedFramePair> try_sync();
    std::size_t camera_depth() const { return camera_queue_.size(); }
    std::size_t lidar_depth() const { return lidar_queue_.size(); }
    std::uint64_t dropped_camera() const { return dropped_camera_; }
    std::uint64_t dropped_lidar() const { return dropped_lidar_; }

private:
    static std::uint64_t delta(std::uint64_t a, std::uint64_t b);
    void trim_old();

    std::uint64_t tolerance_ns_ = 0;
    std::size_t max_queue_ = 0;
    std::deque<FrameMeta> camera_queue_;
    std::deque<FrameMeta> lidar_queue_;
    std::uint64_t dropped_camera_ = 0;
    std::uint64_t dropped_lidar_ = 0;
};

}  // namespace avm
