/**
 * @file camera_adapter_v4l2.cpp
 * @brief V4L2 USB UVC 摄像头适配器实现：YUYV mmap 采集，可输出 RGB888 或紧凑 YUYV raw SensorFrame。
 */
#include "camera_adapter_v4l2.hpp"

#include "avm_config.hpp"
#include "time_utils.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <utility>

CameraAdapterV4L2::CameraAdapterV4L2(std::string device,
                                     std::uint32_t width,
                                     std::uint32_t height,
                                     std::uint32_t fps,
                                     CameraOutputFormat output_format)
    : device_(std::move(device)), width_(width), height_(height), fps_(fps), output_format_(output_format) {}

CameraAdapterV4L2::~CameraAdapterV4L2() {
    stop();
}

int CameraAdapterV4L2::xioctl(int fd, unsigned long request, void* arg) {
    int ret = 0;
    do {
        ret = ::ioctl(fd, request, arg);
    } while (ret == -1 && errno == EINTR);
    return ret;
}

bool CameraAdapterV4L2::open() {
    if (opened_) {
        return true;
    }

    fd_ = ::open(device_.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd_ < 0) {
        std::cerr << "[CameraAdapterV4L2] open failed device=" << device_
                  << " error=" << std::strerror(errno) << "\n";
        return false;
    }

    std::cout << "[CameraAdapterV4L2] open device=" << device_ << "\n";

    if (!query_capability() || !set_format_yuyv() || !set_fps() || !init_mmap()) {
        stop();
        return false;
    }

    opened_ = true;
    return true;
}

bool CameraAdapterV4L2::query_capability() {
    v4l2_capability cap{};
    if (xioctl(fd_, VIDIOC_QUERYCAP, &cap) < 0) {
        std::cerr << "[CameraAdapterV4L2] VIDIOC_QUERYCAP failed: " << std::strerror(errno) << "\n";
        return false;
    }

    const std::uint32_t caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
    if ((caps & V4L2_CAP_VIDEO_CAPTURE) == 0U) {
        std::cerr << "[CameraAdapterV4L2] device does not support V4L2_CAP_VIDEO_CAPTURE\n";
        return false;
    }
    if ((caps & V4L2_CAP_STREAMING) == 0U) {
        std::cerr << "[CameraAdapterV4L2] device does not support V4L2_CAP_STREAMING\n";
        return false;
    }

    std::cout << "[CameraAdapterV4L2] driver=" << cap.driver
              << " card=" << cap.card
              << " bus=" << cap.bus_info << "\n";
    return true;
}

bool CameraAdapterV4L2::set_format_yuyv() {
    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width_;
    fmt.fmt.pix.height = height_;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (xioctl(fd_, VIDIOC_S_FMT, &fmt) < 0) {
        std::cerr << "[CameraAdapterV4L2] VIDIOC_S_FMT YUYV failed: " << std::strerror(errno) << "\n";
        return false;
    }

    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
        std::cerr << "[CameraAdapterV4L2] driver did not accept YUYV format\n";
        return false;
    }

    if (fmt.fmt.pix.width != width_ || fmt.fmt.pix.height != height_) {
        std::cerr << "[CameraAdapterV4L2] warning: driver adjusted size from "
                  << width_ << "x" << height_ << " to "
                  << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << "\n";
        width_ = fmt.fmt.pix.width;
        height_ = fmt.fmt.pix.height;
    }

    bytes_per_line_ = fmt.fmt.pix.bytesperline;
    if (bytes_per_line_ < width_ * 2U) {
        bytes_per_line_ = width_ * 2U;
    }
    size_image_ = fmt.fmt.pix.sizeimage;
    if (size_image_ < bytes_per_line_ * height_) {
        size_image_ = bytes_per_line_ * height_;
    }

    std::cout << "[CameraAdapterV4L2] set format YUYV " << width_ << "x" << height_
              << " bytes_per_line=" << bytes_per_line_
              << " size_image=" << size_image_ << "\n";
    return true;
}

bool CameraAdapterV4L2::set_fps() {
    v4l2_streamparm parm{};
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = fps_;

    if (xioctl(fd_, VIDIOC_S_PARM, &parm) < 0) {
        std::cerr << "[CameraAdapterV4L2] VIDIOC_S_PARM failed: " << std::strerror(errno) << "\n";
        return false;
    }

    const auto num = parm.parm.capture.timeperframe.numerator;
    const auto den = parm.parm.capture.timeperframe.denominator;
    if (num != 0U && den != 0U) {
        std::cout << "[CameraAdapterV4L2] fps requested=" << fps_
                  << " actual=" << static_cast<double>(den) / static_cast<double>(num) << "\n";
    }
    return true;
}

bool CameraAdapterV4L2::init_mmap() {
    v4l2_requestbuffers req{};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd_, VIDIOC_REQBUFS, &req) < 0) {
        std::cerr << "[CameraAdapterV4L2] VIDIOC_REQBUFS failed: " << std::strerror(errno) << "\n";
        return false;
    }
    if (req.count < 2) {
        std::cerr << "[CameraAdapterV4L2] insufficient mmap buffers: " << req.count << "\n";
        return false;
    }

    buffers_.resize(req.count);
    for (std::uint32_t i = 0; i < req.count; ++i) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(fd_, VIDIOC_QUERYBUF, &buf) < 0) {
            std::cerr << "[CameraAdapterV4L2] VIDIOC_QUERYBUF failed index=" << i
                      << " error=" << std::strerror(errno) << "\n";
            return false;
        }

        buffers_[i].length = buf.length;
        buffers_[i].start = ::mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buf.m.offset);
        if (buffers_[i].start == MAP_FAILED) {
            std::cerr << "[CameraAdapterV4L2] mmap failed index=" << i
                      << " error=" << std::strerror(errno) << "\n";
            buffers_[i].start = nullptr;
            return false;
        }
    }

    std::cout << "[CameraAdapterV4L2] mmap buffers=" << buffers_.size() << "\n";
    return true;
}

bool CameraAdapterV4L2::queue_all_buffers() {
    for (std::uint32_t i = 0; i < buffers_.size(); ++i) {
        if (!requeue_buffer(i)) {
            return false;
        }
    }
    return true;
}

bool CameraAdapterV4L2::stream_on() {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd_, VIDIOC_STREAMON, &type) < 0) {
        std::cerr << "[CameraAdapterV4L2] VIDIOC_STREAMON failed: " << std::strerror(errno) << "\n";
        return false;
    }
    std::cout << "[CameraAdapterV4L2] stream on\n";
    return true;
}

bool CameraAdapterV4L2::stream_off() {
    if (fd_ < 0) {
        return true;
    }
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd_, VIDIOC_STREAMOFF, &type) < 0) {
        std::cerr << "[CameraAdapterV4L2] VIDIOC_STREAMOFF failed: " << std::strerror(errno) << "\n";
        return false;
    }
    std::cout << "[CameraAdapterV4L2] stream off\n";
    return true;
}

bool CameraAdapterV4L2::start() {
    if (!opened_ && !open()) {
        return false;
    }
    if (started_) {
        return true;
    }
    if (!queue_all_buffers() || !stream_on()) {
        return false;
    }
    started_ = true;
    return true;
}

bool CameraAdapterV4L2::dequeue_buffer(std::uint32_t& index, std::size_t& bytes_used) {
    pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;

    const int poll_ret = ::poll(&pfd, 1, 2000);
    if (poll_ret < 0) {
        if (errno == EINTR) {
            return false;
        }
        std::cerr << "[CameraAdapterV4L2] poll failed: " << std::strerror(errno) << "\n";
        return false;
    }
    if (poll_ret == 0) {
        std::cerr << "[CameraAdapterV4L2] select timeout\n";
        return false;
    }

    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd_, VIDIOC_DQBUF, &buf) < 0) {
        if (errno == EAGAIN) {
            return false;
        }
        std::cerr << "[CameraAdapterV4L2] VIDIOC_DQBUF failed: " << std::strerror(errno) << "\n";
        return false;
    }

    if (buf.index >= buffers_.size()) {
        std::cerr << "[CameraAdapterV4L2] invalid buffer index=" << buf.index << "\n";
        return false;
    }

    index = buf.index;
    bytes_used = buf.bytesused;
    return true;
}

bool CameraAdapterV4L2::requeue_buffer(std::uint32_t index) {
    if (fd_ < 0 || index >= buffers_.size()) {
        return false;
    }

    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    if (xioctl(fd_, VIDIOC_QBUF, &buf) < 0) {
        std::cerr << "[CameraAdapterV4L2] VIDIOC_QBUF failed index=" << index
                  << " error=" << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool CameraAdapterV4L2::readFrame(SensorFrame& frame) {
    if (!started_) {
        std::cerr << "[CameraAdapterV4L2] readFrame called before start\n";
        return false;
    }

    std::uint32_t index = 0;
    std::size_t bytes_used = 0;
    if (!dequeue_buffer(index, bytes_used)) {
        return false;
    }

    const std::size_t min_expected = static_cast<std::size_t>(width_) * height_ * 2U;
    if (bytes_used < min_expected) {
        std::cerr << "[CameraAdapterV4L2] short YUYV frame bytesused=" << bytes_used
                  << " expected>=" << min_expected << "\n";
        requeue_buffer(index);
        return false;
    }

    const auto* src_yuyv = static_cast<const std::uint8_t*>(buffers_[index].start);
    std::vector<std::uint8_t> payload;
    std::uint32_t output_format = avm::kFormatRgb888;
    std::uint32_t output_stride = width_ * 3U;

    if (output_format_ == CameraOutputFormat::YUYV) {
        payload.resize(static_cast<std::size_t>(width_) * height_ * 2U);
        copy_yuyv_compact(src_yuyv, payload.data(), width_, height_, bytes_per_line_);
        output_format = avm::kFormatYuyv;
        output_stride = width_ * 2U;
    } else {
        payload.resize(static_cast<std::size_t>(width_) * height_ * 3U);
        yuyv_to_rgb888(src_yuyv, payload.data(), width_, height_, bytes_per_line_);
        output_format = avm::kFormatRgb888;
        output_stride = width_ * 3U;
    }

    frame.frame_id = next_frame_id_++;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 0;
    frame.width = width_;
    frame.height = height_;
    frame.format = output_format;
    frame.data_size = static_cast<std::uint32_t>(payload.size());
    frame.stride_bytes = output_stride;
    frame.data = std::move(payload);

    if (frame.frame_id == 1 || frame.frame_id % 30 == 0) {
        std::cout << "[CameraAdapterV4L2] frame_id=" << frame.frame_id
                  << " bytesused=" << bytes_used
                  << " output_format=" << (output_format_ == CameraOutputFormat::YUYV ? "YUYV" : "RGB888")
                  << " output_size=" << frame.data_size
                  << " stride=" << frame.stride_bytes << "\n";
    }

    if (!requeue_buffer(index)) {
        return false;
    }
    return true;
}

void CameraAdapterV4L2::stop() {
    if (started_) {
        stream_off();
        started_ = false;
    }
    cleanup_mmap();
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    opened_ = false;
}

void CameraAdapterV4L2::cleanup_mmap() {
    for (auto& buffer : buffers_) {
        if (buffer.start != nullptr) {
            ::munmap(buffer.start, buffer.length);
            buffer.start = nullptr;
            buffer.length = 0;
        }
    }
    buffers_.clear();
}

std::uint8_t CameraAdapterV4L2::clamp_to_u8(int value) {
    return static_cast<std::uint8_t>(std::max(0, std::min(255, value)));
}

void CameraAdapterV4L2::yuyv_to_rgb888(const std::uint8_t* yuyv,
                                       std::uint8_t* rgb,
                                       std::uint32_t width,
                                       std::uint32_t height,
                                       std::uint32_t bytes_per_line) {
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* src = yuyv + static_cast<std::size_t>(y) * bytes_per_line;
        std::uint8_t* dst = rgb + static_cast<std::size_t>(y) * width * 3U;
        for (std::uint32_t x = 0; x + 1 < width; x += 2) {
            const int y0 = src[0];
            const int u = src[1];
            const int y1 = src[2];
            const int v = src[3];

            const int c0 = y0 - 16;
            const int c1 = y1 - 16;
            const int d = u - 128;
            const int e = v - 128;

            dst[0] = clamp_to_u8((298 * c0 + 409 * e + 128) >> 8);
            dst[1] = clamp_to_u8((298 * c0 - 100 * d - 208 * e + 128) >> 8);
            dst[2] = clamp_to_u8((298 * c0 + 516 * d + 128) >> 8);

            dst[3] = clamp_to_u8((298 * c1 + 409 * e + 128) >> 8);
            dst[4] = clamp_to_u8((298 * c1 - 100 * d - 208 * e + 128) >> 8);
            dst[5] = clamp_to_u8((298 * c1 + 516 * d + 128) >> 8);

            src += 4;
            dst += 6;
        }
    }
}
void CameraAdapterV4L2::copy_yuyv_compact(const std::uint8_t* yuyv,
                                           std::uint8_t* compact,
                                           std::uint32_t width,
                                           std::uint32_t height,
                                           std::uint32_t bytes_per_line) {
    const std::uint32_t compact_stride = width * 2U;
    for (std::uint32_t y = 0; y < height; ++y) {
        const auto* src = yuyv + static_cast<std::size_t>(y) * bytes_per_line;
        auto* dst = compact + static_cast<std::size_t>(y) * compact_stride;
        std::memcpy(dst, src, compact_stride);
    }
}