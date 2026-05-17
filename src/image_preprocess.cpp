/**
 * @file image_preprocess.cpp
 * @brief CPU fallback preprocess implementation: RGB888 resize/normalize and fused YUYV->RGB resize/normalize.
 */
#include "image_preprocess.hpp"

#include <algorithm>
#include <cstddef>

namespace avm {

std::uint8_t clamp_to_u8(int value) {
    return static_cast<std::uint8_t>(std::max(0, std::min(255, value)));
}

namespace {
inline void yuv_to_rgb(int y, int u, int v, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
    const int c = y - 16;
    const int d = u - 128;
    const int e = v - 128;
    r = clamp_to_u8((298 * c + 409 * e + 128) >> 8);
    g = clamp_to_u8((298 * c - 100 * d - 208 * e + 128) >> 8);
    b = clamp_to_u8((298 * c + 516 * d + 128) >> 8);
}
}  // namespace

ResizeIndexPlan make_resize_index_plan(std::uint32_t src_width,
                                        std::uint32_t src_height,
                                        std::uint32_t src_stride_bytes,
                                        std::uint32_t dst_width,
                                        std::uint32_t dst_height) {
    ResizeIndexPlan plan;
    plan.src_width = src_width;
    plan.src_height = src_height;
    plan.src_stride_bytes = src_stride_bytes;
    plan.dst_width = dst_width;
    plan.dst_height = dst_height;
    plan.row_offsets.resize(dst_height);
    plan.rgb_x_offsets.resize(dst_width);
    plan.yuyv_pair_offsets.resize(dst_width);
    plan.yuyv_select_y1.resize(dst_width);

    for (std::uint32_t y = 0; y < dst_height; ++y) {
        const std::uint32_t src_y = y * src_height / dst_height;
        plan.row_offsets[y] = src_y * src_stride_bytes;
    }
    for (std::uint32_t x = 0; x < dst_width; ++x) {
        const std::uint32_t src_x = x * src_width / dst_width;
        const std::uint32_t pair_x = src_x & ~1U;
        plan.rgb_x_offsets[x] = src_x * 3U;
        plan.yuyv_pair_offsets[x] = pair_x * 2U;
        plan.yuyv_select_y1[x] = static_cast<std::uint8_t>(src_x & 1U);
    }
    return plan;
}

void resize_rgb888_to_tensor(const std::uint8_t* rgb,
                             std::uint32_t src_width,
                             std::uint32_t src_height,
                             std::uint32_t src_stride_bytes,
                             std::uint32_t dst_width,
                             std::uint32_t dst_height,
                             float* tensor) {
    if (src_stride_bytes == 0) {
        src_stride_bytes = src_width * 3U;
    }

    for (std::uint32_t y = 0; y < dst_height; ++y) {
        const std::uint32_t src_y = y * src_height / dst_height;
        const auto* src_row = rgb + static_cast<std::size_t>(src_y) * src_stride_bytes;
        for (std::uint32_t x = 0; x < dst_width; ++x) {
            const std::uint32_t src_x = x * src_width / dst_width;
            const std::size_t src_idx = static_cast<std::size_t>(src_x) * 3U;
            const std::size_t dst_idx = (static_cast<std::size_t>(y) * dst_width + x) * 3U;
            tensor[dst_idx + 0] = static_cast<float>(src_row[src_idx + 0]) / 255.0F;
            tensor[dst_idx + 1] = static_cast<float>(src_row[src_idx + 1]) / 255.0F;
            tensor[dst_idx + 2] = static_cast<float>(src_row[src_idx + 2]) / 255.0F;
        }
    }
}

void resize_yuyv_to_tensor(const std::uint8_t* yuyv,
                           std::uint32_t src_width,
                           std::uint32_t src_height,
                           std::uint32_t src_stride_bytes,
                           std::uint32_t dst_width,
                           std::uint32_t dst_height,
                           float* tensor) {
    if (src_stride_bytes == 0) {
        src_stride_bytes = src_width * 2U;
    }

    for (std::uint32_t y = 0; y < dst_height; ++y) {
        const std::uint32_t src_y = y * src_height / dst_height;
        const auto* src_row = yuyv + static_cast<std::size_t>(src_y) * src_stride_bytes;
        for (std::uint32_t x = 0; x < dst_width; ++x) {
            const std::uint32_t src_x = x * src_width / dst_width;
            const std::uint32_t pair_x = src_x & ~1U;
            const auto* px = src_row + static_cast<std::size_t>(pair_x) * 2U;

            const int y0 = px[0];
            const int u = px[1];
            const int y1 = px[2];
            const int v = px[3];
            const int yy = (src_x & 1U) ? y1 : y0;

            std::uint8_t r = 0;
            std::uint8_t g = 0;
            std::uint8_t b = 0;
            yuv_to_rgb(yy, u, v, r, g, b);

            const std::size_t dst_idx = (static_cast<std::size_t>(y) * dst_width + x) * 3U;
            tensor[dst_idx + 0] = static_cast<float>(r) / 255.0F;
            tensor[dst_idx + 1] = static_cast<float>(g) / 255.0F;
            tensor[dst_idx + 2] = static_cast<float>(b) / 255.0F;
        }
    }
}

void resize_rgb888_to_tensor_plan(const std::uint8_t* rgb,
                                  const ResizeIndexPlan& plan,
                                  float* tensor) {
    for (std::uint32_t y = 0; y < plan.dst_height; ++y) {
        const auto* base = rgb + plan.row_offsets[y];
        for (std::uint32_t x = 0; x < plan.dst_width; ++x) {
            const std::uint32_t src_idx = plan.rgb_x_offsets[x];
            const std::size_t dst_idx = (static_cast<std::size_t>(y) * plan.dst_width + x) * 3U;
            tensor[dst_idx + 0] = static_cast<float>(base[src_idx + 0]) / 255.0F;
            tensor[dst_idx + 1] = static_cast<float>(base[src_idx + 1]) / 255.0F;
            tensor[dst_idx + 2] = static_cast<float>(base[src_idx + 2]) / 255.0F;
        }
    }
}

void resize_yuyv_to_tensor_plan(const std::uint8_t* yuyv,
                                const ResizeIndexPlan& plan,
                                float* tensor) {
    for (std::uint32_t y = 0; y < plan.dst_height; ++y) {
        const auto* base = yuyv + plan.row_offsets[y];
        for (std::uint32_t x = 0; x < plan.dst_width; ++x) {
            const auto* px = base + plan.yuyv_pair_offsets[x];
            const int yy = plan.yuyv_select_y1[x] ? px[2] : px[0];
            const int u = px[1];
            const int v = px[3];

            std::uint8_t r = 0;
            std::uint8_t g = 0;
            std::uint8_t b = 0;
            yuv_to_rgb(yy, u, v, r, g, b);

            const std::size_t dst_idx = (static_cast<std::size_t>(y) * plan.dst_width + x) * 3U;
            tensor[dst_idx + 0] = static_cast<float>(r) / 255.0F;
            tensor[dst_idx + 1] = static_cast<float>(g) / 255.0F;
            tensor[dst_idx + 2] = static_cast<float>(b) / 255.0F;
        }
    }
}

void resize_nv12_to_tensor_plan(const std::uint8_t* nv12,
                                std::uint32_t src_width,
                                std::uint32_t src_height,
                                std::uint32_t src_stride_bytes,
                                std::uint32_t uv_stride_bytes,
                                const ResizeIndexPlan& plan,
                                float* tensor) {
    if (src_stride_bytes == 0) {
        src_stride_bytes = src_width;
    }
    if (uv_stride_bytes == 0) {
        uv_stride_bytes = src_stride_bytes;
    }
    const auto* y_plane = nv12;
    const auto* uv_plane = nv12 + static_cast<std::size_t>(src_stride_bytes) * src_height;

    for (std::uint32_t y = 0; y < plan.dst_height; ++y) {
        const std::uint32_t src_y = plan.row_offsets[y] / src_stride_bytes;
        const auto* y_row = y_plane + static_cast<std::size_t>(src_y) * src_stride_bytes;
        const auto* uv_row = uv_plane + static_cast<std::size_t>(src_y / 2U) * uv_stride_bytes;
        for (std::uint32_t x = 0; x < plan.dst_width; ++x) {
            const std::uint32_t src_x = plan.rgb_x_offsets[x] / 3U;
            const int yy = y_row[src_x];
            const std::uint32_t uv_x = src_x & ~1U;
            const int u = uv_row[uv_x + 0U];
            const int v = uv_row[uv_x + 1U];

            std::uint8_t r = 0;
            std::uint8_t g = 0;
            std::uint8_t b = 0;
            yuv_to_rgb(yy, u, v, r, g, b);

            const std::size_t dst_idx = (static_cast<std::size_t>(y) * plan.dst_width + x) * 3U;
            tensor[dst_idx + 0] = static_cast<float>(r) / 255.0F;
            tensor[dst_idx + 1] = static_cast<float>(g) / 255.0F;
            tensor[dst_idx + 2] = static_cast<float>(b) / 255.0F;
        }
    }
}

const char* preprocess_backend_name(PreprocessBackend backend) {
    switch (backend) {
        case PreprocessBackend::CPU_FALLBACK:
            return "cpu_fallback";
        case PreprocessBackend::LIBYUV_COMPAT:
            return "libyuv_compat";
        default:
            return "unknown";
    }
}

void preprocess_to_tensor(const std::uint8_t* data,
                          std::uint32_t format,
                          std::uint32_t src_width,
                          std::uint32_t src_height,
                          std::uint32_t src_stride_bytes,
                          PreprocessBackend backend,
                          const ResizeIndexPlan& plan,
                          float* tensor) {
    (void)backend;
    if (format == 0x52474238U) {
        resize_rgb888_to_tensor_plan(data, plan, tensor);
    } else if (format == 0x59555956U) {
        resize_yuyv_to_tensor_plan(data, plan, tensor);
    } else if (format == 0x4E563132U) {
        const std::uint32_t y_stride = src_stride_bytes == 0 ? src_width : src_stride_bytes;
        resize_nv12_to_tensor_plan(data, src_width, src_height, y_stride, y_stride, plan, tensor);
    }
}

}  // namespace avm

