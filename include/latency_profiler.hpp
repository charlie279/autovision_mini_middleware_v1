/**
 * @file latency_profiler.hpp
 * @brief 延迟统计工具：记录均值、P95 等性能指标。
 */
#pragma once

#include <cstddef>
#include <vector>

class LatencyProfiler {
public:
    void add(double ms);
    double mean() const;
    double percentile(double p) const;
    std::size_t size() const;

private:
    std::vector<double> samples_;
};
