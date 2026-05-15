/**
 * @file file_adapter.cpp
 * @brief 文件输入适配器实现：从 RGB raw 文件中循环读取帧，并输出 SensorFrame。
 */
#include "file_adapter.hpp"

#include "avm_config.hpp"
#include "time_utils.hpp"

#include <iostream>

FileAdapter::FileAdapter(std::string path, std::uint32_t width, std::uint32_t height, std::uint32_t channels)
    : path_(std::move(path)), width_(width), height_(height), channels_(channels) {
    frame_bytes_ = static_cast<std::size_t>(width_) * height_ * channels_;
}

bool FileAdapter::open() {
    input_.open(path_, std::ios::binary);
    if (!input_.is_open()) {
        std::cerr << "[FileAdapter] failed to open " << path_ << "\n";
        return false;
    }
    return true;
}

bool FileAdapter::start() {
    if (!input_.is_open()) {
        return false;
    }
    started_ = true;
    return true;
}

bool FileAdapter::rewind_file() {
    input_.clear();
    input_.seekg(0, std::ios::beg);
    return static_cast<bool>(input_);
}

bool FileAdapter::readFrame(SensorFrame& frame) {
    if (!started_) {
        return false;
    }

    std::vector<std::uint8_t> payload(frame_bytes_);
    input_.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payload.size()));

    if (input_.gcount() != static_cast<std::streamsize>(payload.size())) {
        if (!rewind_file()) {
            return false;
        }
        input_.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
        if (input_.gcount() != static_cast<std::streamsize>(payload.size())) {
            std::cerr << "[FileAdapter] input file is too small for one frame\n";
            return false;
        }
    }

    frame.frame_id = next_frame_id_++;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 2;
    frame.width = width_;
    frame.height = height_;
    frame.format = avm::kFormatRgb888;
    frame.data_size = static_cast<std::uint32_t>(payload.size());
    frame.data = std::move(payload);
    return true;
}

void FileAdapter::stop() {
    started_ = false;
    input_.close();
}