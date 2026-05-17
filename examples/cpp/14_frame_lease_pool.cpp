/**
 * @file 14_frame_lease_pool.cpp
 * @brief 示例：验证 FrameLeasePool 的 acquire/publish/release/reclaim 生命周期。
 */
#include "frame_lease.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    constexpr std::uint64_t kTtlNs = 100000000ULL;
    avm::FrameLeasePool pool(3, kTtlNs);
    std::filesystem::create_directories("examples/logs");
    std::ofstream csv("examples/logs/frame_lease_pool.csv", std::ios::out | std::ios::trunc);
    csv << "step,index,frame_id,state,ref_count,free_count,busy_count\n";

    auto dump = [&](const char* step, std::uint32_t idx) {
        const auto& s = pool.slot(idx);
        csv << step << ',' << idx << ',' << s.frame_id << ','
            << avm::FrameLeasePool::state_name(s.state) << ',' << s.ref_count << ','
            << pool.free_count() << ',' << pool.busy_count() << '\n';
        std::cout << "[14_frame_lease_pool] step=" << step
                  << " index=" << idx
                  << " frame_id=" << s.frame_id
                  << " state=" << avm::FrameLeasePool::state_name(s.state)
                  << " refs=" << s.ref_count
                  << " free=" << pool.free_count()
                  << " busy=" << pool.busy_count() << "\n";
    };

    const std::int32_t a = pool.acquire(1, 0);
    const std::int32_t b = pool.acquire(2, 10);
    const std::int32_t c = pool.acquire(3, 20);
    if (a < 0 || b < 0 || c < 0) {
        std::cerr << "[14_frame_lease_pool] initial acquire failed\n";
        return 1;
    }
    dump("acquire_a", static_cast<std::uint32_t>(a));
    dump("acquire_b", static_cast<std::uint32_t>(b));
    dump("acquire_c", static_cast<std::uint32_t>(c));

    pool.publish(static_cast<std::uint32_t>(a), 2, 30);
    pool.publish(static_cast<std::uint32_t>(b), 1, 40);
    pool.publish(static_cast<std::uint32_t>(c), 1, 50);
    dump("publish_a", static_cast<std::uint32_t>(a));

    const std::int32_t no_slot = pool.acquire(4, 60);
    if (no_slot >= 0) {
        std::cerr << "[14_frame_lease_pool] expected acquire failure when all slots are busy\n";
        return 2;
    }

    pool.release(static_cast<std::uint32_t>(a));
    dump("release_a_once", static_cast<std::uint32_t>(a));
    pool.release(static_cast<std::uint32_t>(a));
    dump("release_a_twice", static_cast<std::uint32_t>(a));

    const std::int32_t d = pool.acquire(4, 70);
    if (d < 0) {
        std::cerr << "[14_frame_lease_pool] expected acquire success after release\n";
        return 3;
    }
    pool.publish(static_cast<std::uint32_t>(d), 1, 80);
    dump("publish_d", static_cast<std::uint32_t>(d));

    const std::uint32_t reclaimed = pool.reclaim_expired(1000000000ULL);
    std::cout << "[14_frame_lease_pool] reclaimed=" << reclaimed << "\n";
    const auto& stats = pool.stats();
    std::cout << "[14_frame_lease_pool] stats acquire_ok=" << stats.acquire_ok
              << " acquire_fail=" << stats.acquire_fail
              << " publish_ok=" << stats.publish_ok
              << " release_ok=" << stats.release_ok
              << " reclaimed=" << stats.reclaimed
              << " invalid_ops=" << stats.invalid_ops << "\n";

    if (stats.acquire_ok != 4 || stats.acquire_fail != 1 || stats.reclaimed == 0) {
        return 4;
    }
    std::cout << "[14_frame_lease_pool] done csv=examples/logs/frame_lease_pool.csv\n";
    return 0;
}
