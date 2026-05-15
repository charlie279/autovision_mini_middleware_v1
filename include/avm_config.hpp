/**
 * @file avm_config.hpp
 * @brief 全局配置常量：共享内存名称、缓冲区大小、Ring 容量、控制 Socket 路径和像素格式。
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace avm {

constexpr const char* kFramePoolName = "/avm_frame_pool";
constexpr const char* kFrameMetaRingName = "/avm_frame_meta_ring";
constexpr const char* kTensorPoolName = "/avm_tensor_pool";
constexpr const char* kTensorMetaRingName = "/avm_tensor_meta_ring";
constexpr const char* kStatusName = "/avm_runtime_status";
constexpr const char* kControlSockPath = "/tmp/avm_control.sock";

constexpr std::uint32_t kDefaultWidth = 640;
constexpr std::uint32_t kDefaultHeight = 480;
constexpr std::uint32_t kDefaultChannels = 3;
constexpr std::uint32_t kDefaultFps = 30;

constexpr std::size_t kFramePoolCount = 8;
constexpr std::size_t kFrameBufferSize = 1280 * 720 * 3;

constexpr std::uint32_t kTensorWidth = 320;
constexpr std::uint32_t kTensorHeight = 320;
constexpr std::uint32_t kTensorChannels = 3;
constexpr std::size_t kTensorPoolCount = 8;
constexpr std::size_t kTensorBufferSize = kTensorWidth * kTensorHeight * kTensorChannels * sizeof(float);

constexpr std::size_t kRingCapacity = 64;

constexpr std::uint32_t kFormatRgb888 = 0x52474238U;       // 'RGB8'
constexpr std::uint32_t kFormatYuyv = 0x59555956U;         // 'YUYV'
constexpr std::uint32_t kFormatNv12 = 0x4E563132U;         // 'NV12'
constexpr std::uint32_t kFormatLidarFloat32 = 0x4C493332U; // 'LI32'

}  // namespace avm