/**
 * @file system_tuning.hpp
 * @brief CPU affinity and thread priority helpers used by benchmark examples.
 */
#pragma once

#include <cstdint>

namespace avm {

bool set_current_thread_affinity(int cpu_id);
bool set_current_thread_fifo_priority(int priority);
std::uint64_t run_cpu_spin_benchmark(std::uint64_t iterations);

}  // namespace avm

