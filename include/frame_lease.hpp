/**
 * @file frame_lease.hpp
 * @brief Lightweight buffer lease/ref-count model inspired by video middleware zero-copy lifecycles.
 *
 * This module does not implement DMA-BUF. It provides a VMware-friendly simulation of
 * producer acquire -> publish -> consumer retain/release -> timeout reclaim semantics.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace avm {

enum class LeaseState : std::uint32_t {
    FREE = 0,
    WRITING = 1,
    PUBLISHED = 2,
    RECLAIMED = 3
};

struct FrameLeaseSlot {
    std::uint32_t index = 0;
    std::uint64_t frame_id = 0;
    std::uint32_t ref_count = 0;
    std::uint64_t acquire_ns = 0;
    std::uint64_t expire_ns = 0;
    LeaseState state = LeaseState::FREE;
};

struct FrameLeaseStats {
    std::uint64_t acquire_ok = 0;
    std::uint64_t acquire_fail = 0;
    std::uint64_t publish_ok = 0;
    std::uint64_t retain_ok = 0;
    std::uint64_t release_ok = 0;
    std::uint64_t reclaimed = 0;
    std::uint64_t invalid_ops = 0;
};

class FrameLeasePool {
public:
    explicit FrameLeasePool(std::uint32_t slot_count = 8, std::uint64_t default_ttl_ns = 1000000000ULL);

    std::int32_t acquire(std::uint64_t frame_id, std::uint64_t now_ns);
    bool publish(std::uint32_t index, std::uint32_t initial_ref_count, std::uint64_t now_ns);
    bool retain(std::uint32_t index);
    bool release(std::uint32_t index);
    std::uint32_t reclaim_expired(std::uint64_t now_ns);

    const FrameLeaseSlot& slot(std::uint32_t index) const;
    const std::vector<FrameLeaseSlot>& slots() const { return slots_; }
    const FrameLeaseStats& stats() const { return stats_; }
    std::uint32_t free_count() const;
    std::uint32_t busy_count() const;

    static const char* state_name(LeaseState state);

private:
    std::vector<FrameLeaseSlot> slots_;
    std::uint64_t default_ttl_ns_ = 0;
    FrameLeaseStats stats_;
};

}  // namespace avm
