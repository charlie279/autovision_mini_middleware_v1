/**
 * @file frame_lease.cpp
 * @brief Lightweight buffer lease/ref-count implementation.
 */
#include "frame_lease.hpp"

#include <stdexcept>

namespace avm {

FrameLeasePool::FrameLeasePool(std::uint32_t slot_count, std::uint64_t default_ttl_ns)
    : slots_(slot_count), default_ttl_ns_(default_ttl_ns) {
    for (std::uint32_t i = 0; i < slot_count; ++i) {
        slots_[i].index = i;
    }
}

std::int32_t FrameLeasePool::acquire(std::uint64_t frame_id, std::uint64_t now_ns) {
    for (auto& s : slots_) {
        if (s.state == LeaseState::FREE || s.state == LeaseState::RECLAIMED) {
            s.frame_id = frame_id;
            s.ref_count = 0;
            s.acquire_ns = now_ns;
            s.expire_ns = now_ns + default_ttl_ns_;
            s.state = LeaseState::WRITING;
            ++stats_.acquire_ok;
            return static_cast<std::int32_t>(s.index);
        }
    }
    ++stats_.acquire_fail;
    return -1;
}

bool FrameLeasePool::publish(std::uint32_t index, std::uint32_t initial_ref_count, std::uint64_t now_ns) {
    if (index >= slots_.size() || initial_ref_count == 0) {
        ++stats_.invalid_ops;
        return false;
    }
    auto& s = slots_[index];
    if (s.state != LeaseState::WRITING) {
        ++stats_.invalid_ops;
        return false;
    }
    s.ref_count = initial_ref_count;
    s.expire_ns = now_ns + default_ttl_ns_;
    s.state = LeaseState::PUBLISHED;
    ++stats_.publish_ok;
    return true;
}

bool FrameLeasePool::retain(std::uint32_t index) {
    if (index >= slots_.size()) {
        ++stats_.invalid_ops;
        return false;
    }
    auto& s = slots_[index];
    if (s.state != LeaseState::PUBLISHED || s.ref_count == 0) {
        ++stats_.invalid_ops;
        return false;
    }
    ++s.ref_count;
    ++stats_.retain_ok;
    return true;
}

bool FrameLeasePool::release(std::uint32_t index) {
    if (index >= slots_.size()) {
        ++stats_.invalid_ops;
        return false;
    }
    auto& s = slots_[index];
    if (s.state != LeaseState::PUBLISHED || s.ref_count == 0) {
        ++stats_.invalid_ops;
        return false;
    }
    --s.ref_count;
    ++stats_.release_ok;
    if (s.ref_count == 0) {
        s.state = LeaseState::FREE;
        s.frame_id = 0;
        s.acquire_ns = 0;
        s.expire_ns = 0;
    }
    return true;
}

std::uint32_t FrameLeasePool::reclaim_expired(std::uint64_t now_ns) {
    std::uint32_t count = 0;
    for (auto& s : slots_) {
        if (s.state == LeaseState::PUBLISHED && s.ref_count > 0 && s.expire_ns <= now_ns) {
            s.state = LeaseState::RECLAIMED;
            s.ref_count = 0;
            ++count;
        }
    }
    stats_.reclaimed += count;
    return count;
}

const FrameLeaseSlot& FrameLeasePool::slot(std::uint32_t index) const {
    if (index >= slots_.size()) {
        throw std::out_of_range("FrameLeasePool::slot index out of range");
    }
    return slots_[index];
}

std::uint32_t FrameLeasePool::free_count() const {
    std::uint32_t count = 0;
    for (const auto& s : slots_) {
        if (s.state == LeaseState::FREE || s.state == LeaseState::RECLAIMED) {
            ++count;
        }
    }
    return count;
}

std::uint32_t FrameLeasePool::busy_count() const {
    return static_cast<std::uint32_t>(slots_.size()) - free_count();
}

const char* FrameLeasePool::state_name(LeaseState state) {
    switch (state) {
        case LeaseState::FREE:
            return "FREE";
        case LeaseState::WRITING:
            return "WRITING";
        case LeaseState::PUBLISHED:
            return "PUBLISHED";
        case LeaseState::RECLAIMED:
            return "RECLAIMED";
        default:
            return "UNKNOWN";
    }
}

}  // namespace avm