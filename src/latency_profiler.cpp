/**
 * @file latency_profiler.cpp
 * @brief 延迟统计实现：支持均值和百分位统计。
 */
#include "latency_profiler.hpp"

#include <algorithm>
#include <numeric>

void LatencyProfiler::add(double ms) {
    samples_.push_back(ms);
}

double LatencyProfiler::mean() const {
    if (samples_.empty()) {
        return 0.0;
    }
    const double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    return sum / static_cast<double>(samples_.size());
}

double LatencyProfiler::percentile(double p) const {
    if (samples_.empty()) {
        return 0.0;
    }

    std::vector<double> sorted = samples_;
    std::sort(sorted.begin(), sorted.end());

    const double index = (p / 100.0) * static_cast<double>(sorted.size() - 1);
    const auto low = static_cast<std::size_t>(index);
    const auto high = std::min(low + 1, sorted.size() - 1);
    const double ratio = index - static_cast<double>(low);
    return sorted[low] * (1.0 - ratio) + sorted[high] * ratio;
}

std::size_t LatencyProfiler::size() const {
    return samples_.size();
}
