/**
 * @file record_replay.cpp
 * @brief Frame record/replay implementation.
 */
#include "record_replay.hpp"

#include <iostream>

namespace avm {

bool FrameRecorder::open(const std::string& path) {
    close();
    out_.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
    count_ = 0;
    if (!out_.is_open()) {
        std::cerr << "[FrameRecorder] open failed path=" << path << "\n";
        return false;
    }
    return true;
}

bool FrameRecorder::write(const SensorFrame& frame, std::uint32_t crc32) {
    if (!out_.is_open()) {
        return false;
    }
    RecordFrameHeader h{};
    h.frame_id = frame.frame_id;
    h.timestamp_ns = frame.timestamp_ns;
    h.sensor_type = frame.sensor_type;
    h.width = frame.width;
    h.height = frame.height;
    h.format = frame.format;
    h.data_size = frame.data_size;
    h.stride_bytes = frame.stride_bytes;
    h.crc32 = crc32;
    out_.write(reinterpret_cast<const char*>(&h), sizeof(h));
    out_.write(reinterpret_cast<const char*>(frame.data.data()), static_cast<std::streamsize>(frame.data.size()));
    if (!out_) {
        return false;
    }
    ++count_;
    return true;
}

void FrameRecorder::close() {
    if (out_.is_open()) {
        out_.close();
    }
}

FrameReplayAdapter::FrameReplayAdapter(std::string path) : path_(std::move(path)) {}

bool FrameReplayAdapter::open() {
    in_.open(path_, std::ios::binary | std::ios::in);
    if (!in_.is_open()) {
        std::cerr << "[FrameReplayAdapter] open failed path=" << path_ << "\n";
        return false;
    }
    return true;
}

bool FrameReplayAdapter::start() {
    started_ = in_.is_open();
    return started_;
}

bool FrameReplayAdapter::readFrame(SensorFrame& frame) {
    if (!started_ || !in_.is_open()) {
        return false;
    }
    RecordFrameHeader h{};
    if (!in_.read(reinterpret_cast<char*>(&h), sizeof(h))) {
        return false;
    }
    if (h.magic != 0x41565246U || h.header_size != sizeof(RecordFrameHeader)) {
        std::cerr << "[FrameReplayAdapter] invalid frame header\n";
        return false;
    }
    frame = SensorFrame{};
    frame.frame_id = h.frame_id;
    frame.timestamp_ns = h.timestamp_ns;
    frame.sensor_type = h.sensor_type;
    frame.width = h.width;
    frame.height = h.height;
    frame.format = h.format;
    frame.data_size = h.data_size;
    frame.stride_bytes = h.stride_bytes;
    frame.data.resize(h.data_size);
    if (!in_.read(reinterpret_cast<char*>(frame.data.data()), h.data_size)) {
        return false;
    }
    return true;
}

void FrameReplayAdapter::stop() {
    started_ = false;
    if (in_.is_open()) {
        in_.close();
    }
}

}  // namespace avm

