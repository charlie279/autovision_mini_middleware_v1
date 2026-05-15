/**
 * @file crc32.hpp
 * @brief CRC32 校验函数声明：用于帧完整性检查和异常注入验证。
 */
#pragma once

#include <cstddef>
#include <cstdint>

std::uint32_t crc32_compute(const void* data, std::size_t length);