/**
 * @file file_adapter.hpp
 * @brief 文件输入适配器：从 prepare_input.sh 生成的 RGB raw 文件中循环读取固定尺寸帧。
 */
#pragma once

#include "sensor_adapter.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

class FileAdapter final : public SensorAdapter {
public:
    FileAdapter(std::string path, std::uint32_t width, std::uint32_t height, std::uint32_t channels);

    bool open() override;
    bool start() override;
    bool readFrame(SensorFrame& frame) override;
    void stop() override;

private:
    bool rewind_file();

    std::string path_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t channels_ = 0;
    std::size_t frame_bytes_ = 0;
    std::ifstream input_;
    std::uint64_t next_frame_id_ = 1;
    bool started_ = false;
};
