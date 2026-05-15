/**
 * @file 00_crc32.cpp
 * @brief 示例：直接调用 crc32_compute()，理解 payload 完整性校验接口。
 */
#include "crc32.hpp"

#include <cstdint>
#include <iostream>
#include <string>

int main() {
    const std::string payload = "AutoVision Mini Middleware";
    const std::uint32_t crc = crc32_compute(payload.data(), payload.size());

    std::cout << "[00_crc32] payload=\"" << payload << "\"\n";
    std::cout << "[00_crc32] crc32=0x" << std::hex << crc << std::dec << "\n";
    return 0;
}