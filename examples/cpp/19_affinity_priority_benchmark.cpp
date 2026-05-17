#include "system_tuning.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    int cpu = -1;
    int priority = 0;
    std::uint64_t iterations = 20'000'000ULL;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string k = argv[i];
        if (k == "--cpu") cpu = std::stoi(argv[++i]);
        else if (k == "--priority") priority = std::stoi(argv[++i]);
        else if (k == "--iterations") iterations = std::stoull(argv[++i]);
    }
    const bool affinity_ok = avm::set_current_thread_affinity(cpu);
    const bool priority_ok = avm::set_current_thread_fifo_priority(priority);
    const auto t0 = std::chrono::steady_clock::now();
    const auto checksum = avm::run_cpu_spin_benchmark(iterations);
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "[19_affinity_priority_benchmark] cpu=" << cpu
              << " priority=" << priority
              << " affinity_ok=" << (affinity_ok ? "true" : "false")
              << " priority_ok=" << (priority_ok ? "true" : "false")
              << " iterations=" << iterations
              << " elapsed_ms=" << ms
              << " checksum=" << checksum << "\n";
    return 0;
}
