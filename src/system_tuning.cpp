
/**
 * @file system_tuning.cpp
 * @brief CPU affinity and scheduling-priority helpers.
 */
#include "system_tuning.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <sched.h>

namespace avm {

bool set_current_thread_affinity(int cpu_id) {
    if (cpu_id < 0) {
        return true;
    }
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    const int rc = pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
    if (rc != 0) {
        std::cerr << "[system_tuning] set affinity cpu=" << cpu_id << " failed: " << std::strerror(rc) << "\n";
        return false;
    }
    return true;
}

bool set_current_thread_fifo_priority(int priority) {
    if (priority <= 0) {
        return true;
    }
    sched_param param{};
    param.sched_priority = priority;
    const int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (rc != 0) {
        std::cerr << "[system_tuning] set SCHED_FIFO priority=" << priority
                  << " failed: " << std::strerror(rc) << " (non-root is expected to fail)\n";
        return false;
    }
    return true;
}

std::uint64_t run_cpu_spin_benchmark(std::uint64_t iterations) {
    volatile std::uint64_t acc = 0;
    for (std::uint64_t i = 0; i < iterations; ++i) {
        acc = (acc * 1315423911ULL) ^ (i + 0x9e3779b97f4a7c15ULL);
    }
    return acc;
}

}  // namespace avm
