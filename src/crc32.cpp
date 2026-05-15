/**
 * @file crc32.cpp
 * @brief CRC32 实现：对帧 payload 计算校验值，用于 preprocess_node 完整性检查。
 */
#include "crc32.hpp"

std::uint32_t crc32_compute(const void* data, std::size_t length) {
    static std::uint32_t table[256];
    static bool initialized = false;

    if (!initialized) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1U) ? (0xEDB88320U ^ (c >> 1U)) : (c >> 1U);
            }
            table[i] = c;
        }
        initialized = true;
    }

    std::uint32_t crc = 0xFFFFFFFFU;
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < length; ++i) {
        crc = table[(crc ^ bytes[i]) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}