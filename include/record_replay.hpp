/**
 * @file record_replay.hpp
 * @brief Minimal binary record/replay format for SensorFrame streams.
 */
#pragma once

#include "sensor_adapter.hpp"
#include "sensor_frame.hpp"

#include <cstdint>
#include <fstream>
#include <string>

namespace avm {

struct RecordFrameHeader {
    std::uint32_t magic = 0x41565246U;  // 'AVRF'
    std::uint32_t header_size = sizeof(RecordFrameHeader);
    std::uint64_t frame_id = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint32_t sensor_type = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t format = 0;
    std::uint32_t data_size = 0;
    std::uint32_t stride_bytes = 0;
    std::uint32_t crc32 = 0;
};

class FrameRecorder {
public:
    bool open(const std::string& path);
    bool write(const SensorFrame& frame, std::uint32_t crc32);
    void close();
    std::uint64_t count() const { return count_; }

private:
    std::ofstream out_;
    std::uint64_t count_ = 0;
};

class FrameReplayAdapter final : public SensorAdapter {
public:
    explicit FrameReplayAdapter(std::string path);
    bool open() override;
    bool start() override;
    bool readFrame(SensorFrame& frame) override;
    void stop() override;

private:
    std::string path_;
    std::ifstream in_;
    bool started_ = false;
};

}  // namespace avm
