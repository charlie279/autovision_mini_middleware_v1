/**
 * @file camera_adapter_v4l2.hpp
 * @brief V4L2 USB UVC 摄像头适配器：使用 ioctl + mmap 采集 YUYV，可输出 YUYV raw 或 RGB888 SensorFrame。
 */
#pragma once

#include "sensor_adapter.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class CameraOutputFormat : std::uint32_t {
    RGB888 = 0,
    YUYV = 1
};

class CameraAdapterV4L2 final : public SensorAdapter {
public:
    CameraAdapterV4L2(std::string device,
                      std::uint32_t width,
                      std::uint32_t height,
                      std::uint32_t fps = 30,
                      CameraOutputFormat output_format = CameraOutputFormat::RGB888);
    ~CameraAdapterV4L2() override;

    bool open() override;
    bool start() override;
    bool readFrame(SensorFrame& frame) override;
    void stop() override;

private:
    struct Buffer {
        void* start = nullptr;
        std::size_t length = 0;
    };

    bool query_capability();
    bool set_format_yuyv();
    bool set_fps();
    bool init_mmap();
    bool queue_all_buffers();
    bool stream_on();
    bool stream_off();
    bool dequeue_buffer(std::uint32_t& index, std::size_t& bytes_used);
    bool requeue_buffer(std::uint32_t index);
    void cleanup_mmap();

    static int xioctl(int fd, unsigned long request, void* arg);
    static std::uint8_t clamp_to_u8(int value);
    static void yuyv_to_rgb888(const std::uint8_t* yuyv,
                               std::uint8_t* rgb,
                               std::uint32_t width,
                               std::uint32_t height,
                               std::uint32_t bytes_per_line);
    static void copy_yuyv_compact(const std::uint8_t* yuyv,
                                  std::uint8_t* compact,
                                  std::uint32_t width,
                                  std::uint32_t height,
                                  std::uint32_t bytes_per_line);

    std::string device_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t fps_ = 30;
    CameraOutputFormat output_format_ = CameraOutputFormat::RGB888;
    std::uint32_t bytes_per_line_ = 0;
    std::uint32_t size_image_ = 0;

    int fd_ = -1;
    std::vector<Buffer> buffers_;
    std::uint64_t next_frame_id_ = 1;
    bool opened_ = false;
    bool started_ = false;
};
