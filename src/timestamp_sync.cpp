/**
 * @file timestamp_sync.cpp
 * @brief TimestampSync implementation.
 */
#include "timestamp_sync.hpp"

#include <limits>

namespace avm {

TimestampSync::TimestampSync(std::uint64_t tolerance_ns, std::size_t max_queue)
    : tolerance_ns_(tolerance_ns), max_queue_(max_queue) {}

std::uint64_t TimestampSync::delta(std::uint64_t a, std::uint64_t b) {
    return a > b ? a - b : b - a;
}

void TimestampSync::push_camera(const FrameMeta& meta) {
    camera_queue_.push_back(meta);
    if (camera_queue_.size() > max_queue_) {
        camera_queue_.pop_front();
        ++dropped_camera_;
    }
}

void TimestampSync::push_lidar(const FrameMeta& meta) {
    lidar_queue_.push_back(meta);
    if (lidar_queue_.size() > max_queue_) {
        lidar_queue_.pop_front();
        ++dropped_lidar_;
    }
}

void TimestampSync::trim_old() {
    while (camera_queue_.size() > max_queue_) {
        camera_queue_.pop_front();
        ++dropped_camera_;
    }
    while (lidar_queue_.size() > max_queue_) {
        lidar_queue_.pop_front();
        ++dropped_lidar_;
    }
}

std::optional<SyncedFramePair> TimestampSync::try_sync() {
    trim_old();
    if (camera_queue_.empty() || lidar_queue_.empty()) {
        return std::nullopt;
    }

    std::size_t best_cam = 0;
    std::size_t best_lidar = 0;
    std::uint64_t best_delta = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t i = 0; i < camera_queue_.size(); ++i) {
        for (std::size_t j = 0; j < lidar_queue_.size(); ++j) {
            const std::uint64_t d = delta(camera_queue_[i].timestamp_ns, lidar_queue_[j].timestamp_ns);
            if (d < best_delta) {
                best_delta = d;
                best_cam = i;
                best_lidar = j;
            }
        }
    }

    if (best_delta > tolerance_ns_) {
        if (camera_queue_.front().timestamp_ns < lidar_queue_.front().timestamp_ns) {
            camera_queue_.pop_front();
            ++dropped_camera_;
        } else {
            lidar_queue_.pop_front();
            ++dropped_lidar_;
        }
        return std::nullopt;
    }

    SyncedFramePair pair{camera_queue_[best_cam], lidar_queue_[best_lidar], best_delta};
    camera_queue_.erase(camera_queue_.begin(), camera_queue_.begin() + static_cast<long>(best_cam) + 1);
    lidar_queue_.erase(lidar_queue_.begin(), lidar_queue_.begin() + static_cast<long>(best_lidar) + 1);
    return pair;
}

}  // namespace avm

