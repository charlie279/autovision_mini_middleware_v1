/**
 * @file 01_latency_profiler.cpp
 * @brief 示例：调用 LatencyProfiler，理解 mean/P95 延迟统计接口。
 */
#include "latency_profiler.hpp"

#include <iostream>

int main() {
    LatencyProfiler profiler;
    profiler.add(4.2);
    profiler.add(5.1);
    profiler.add(4.8);
    profiler.add(8.0);
    profiler.add(4.5);

    std::cout << "[01_latency_profiler] samples=" << profiler.size() << "\n";
    std::cout << "[01_latency_profiler] mean=" << profiler.mean() << " ms\n";
    std::cout << "[01_latency_profiler] p95=" << profiler.percentile(95.0) << " ms\n";
    return 0;
}
