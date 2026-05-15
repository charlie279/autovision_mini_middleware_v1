/**
 * @file runtime_factory.cpp
 * @brief Runtime backend 工厂实现。
 */
#include "runtime_factory.hpp"

#include "dummy_runtime.hpp"
#include "rknn_runtime_stub.hpp"

#include <iostream>
#include <memory>

std::unique_ptr<InferenceRuntime> create_runtime(const std::string& backend) {
    if (backend == "dummy") {
        return std::make_unique<DummyRuntime>();
    }
    if (backend == "rknn_stub") {
        return std::make_unique<RknnRuntimeStub>();
    }

    std::cerr << "[runtime_factory] unknown backend=" << backend << ", fallback to dummy\n";
    return std::make_unique<DummyRuntime>();
}