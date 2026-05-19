/**
 * @file dds_shm_frame_meta.cpp
 * @brief V2.5 DDS+SHM mixed-route descriptor serialization implementation.
 */
#include "dds_shm_frame_meta.hpp"

#include "avm_config.hpp"
#include "crc32.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace avm {
namespace {
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFU));
    }
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFU));
    }
}

bool read_u32(const std::vector<std::uint8_t>& in, std::size_t& off, std::uint32_t& v) {
    if (off + 4U > in.size()) {
        return false;
    }
    v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<std::uint32_t>(in[off++]) << (i * 8);
    }
    return true;
}

bool read_u64(const std::vector<std::uint8_t>& in, std::size_t& off, std::uint64_t& v) {
    if (off + 8U > in.size()) {
        return false;
    }
    v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<std::uint64_t>(in[off++]) << (i * 8);
    }
    return true;
}

void append_fixed_name(std::vector<std::uint8_t>& out, const char* name) {
    for (std::size_t i = 0; i < kDdsShmNameMax; ++i) {
        out.push_back(static_cast<std::uint8_t>(name[i]));
    }
}

bool read_fixed_name(const std::vector<std::uint8_t>& in, std::size_t& off, char* name) {
    if (off + kDdsShmNameMax > in.size()) {
        return false;
    }
    std::memset(name, 0, kDdsShmNameMax);
    for (std::size_t i = 0; i < kDdsShmNameMax; ++i) {
        name[i] = static_cast<char>(in[off++]);
    }
    name[kDdsShmNameMax - 1U] = '\0';
    return true;
}
}  // namespace

std::string dds_shm_normalized_format(const std::string& format) {
    if (format == "rgb" || format == "rgb888" || format == "RGB" || format == "RGB888") {
        return "rgb";
    }
    if (format == "nv12" || format == "NV12") {
        return "nv12";
    }
    return "yuyv";
}

std::uint32_t dds_shm_bytes_per_pixel(const std::string& format) {
    const auto f = dds_shm_normalized_format(format);
    if (f == "rgb") {
        return 3U;
    }
    if (f == "nv12") {
        return 1U;  // descriptor helper uses integer bpp; NV12 payload size should be provided by frame.data_size.
    }
    return 2U;
}

std::uint32_t dds_shm_format_code(const std::string& format) {
    const auto f = dds_shm_normalized_format(format);
    if (f == "rgb") {
        return kFormatRgb888;
    }
    if (f == "nv12") {
        return kFormatNv12;
    }
    return kFormatYuyv;
}

std::string dds_shm_format_to_string(std::uint32_t format_code) {
    if (format_code == kFormatRgb888) {
        return "rgb";
    }
    if (format_code == kFormatNv12) {
        return "nv12";
    }
    if (format_code == kFormatYuyv) {
        return "yuyv";
    }
    return "unknown";
}

std::size_t dds_shm_expected_payload_size(const SharedFrameDescriptor& desc) {
    if (desc.payload_size != 0U) {
        return static_cast<std::size_t>(desc.payload_size);
    }
    return static_cast<std::size_t>(desc.width) * desc.height * desc.bytes_per_pixel;
}

std::vector<std::uint8_t> serialize_shared_frame_descriptor(const SharedFrameDescriptor& desc) {
    std::vector<std::uint8_t> out;
    out.reserve(4U * 8U + 8U * 7U + kDdsShmNameMax);
    append_u32(out, kDdsShmMetaMagic);
    append_u32(out, kDdsShmMetaVersion);
    append_u64(out, desc.seq);
    append_u64(out, desc.timestamp_us);
    append_u32(out, desc.width);
    append_u32(out, desc.height);
    append_u32(out, desc.format);
    append_u32(out, desc.bytes_per_pixel);
    append_u64(out, desc.payload_size);
    append_u32(out, desc.payload_crc32);
    append_u32(out, desc.buffer_index);
    append_u32(out, desc.buffer_count);
    append_u64(out, desc.buffer_size);
    append_u64(out, desc.offset);
    append_u32(out, desc.alive_counter);
    append_fixed_name(out, desc.shm_name);
    return out;
}

bool deserialize_shared_frame_descriptor(const std::vector<std::uint8_t>& bytes,
                                         SharedFrameDescriptor& desc,
                                         std::string* error) {
    std::size_t off = 0;
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    if (!read_u32(bytes, off, magic) || magic != kDdsShmMetaMagic) {
        if (error) *error = "bad dds-shm descriptor magic";
        return false;
    }
    if (!read_u32(bytes, off, version) || version != kDdsShmMetaVersion) {
        if (error) *error = "unsupported dds-shm descriptor version";
        return false;
    }
    desc = SharedFrameDescriptor{};
    if (!read_u64(bytes, off, desc.seq) || !read_u64(bytes, off, desc.timestamp_us) ||
        !read_u32(bytes, off, desc.width) || !read_u32(bytes, off, desc.height) ||
        !read_u32(bytes, off, desc.format) || !read_u32(bytes, off, desc.bytes_per_pixel) ||
        !read_u64(bytes, off, desc.payload_size) || !read_u32(bytes, off, desc.payload_crc32) ||
        !read_u32(bytes, off, desc.buffer_index) || !read_u32(bytes, off, desc.buffer_count) ||
        !read_u64(bytes, off, desc.buffer_size) || !read_u64(bytes, off, desc.offset) ||
        !read_u32(bytes, off, desc.alive_counter) || !read_fixed_name(bytes, off, desc.shm_name)) {
        if (error) *error = "truncated dds-shm descriptor";
        return false;
    }
    if (desc.buffer_count == 0U || desc.buffer_index >= desc.buffer_count) {
        if (error) {
            std::ostringstream oss;
            oss << "invalid buffer index/count index=" << desc.buffer_index << " count=" << desc.buffer_count;
            *error = oss.str();
        }
        return false;
    }
    if (desc.payload_size == 0U || desc.payload_size > desc.buffer_size) {
        if (error) {
            std::ostringstream oss;
            oss << "invalid payload_size=" << desc.payload_size << " buffer_size=" << desc.buffer_size;
            *error = oss.str();
        }
        return false;
    }
    if (desc.shm_name[0] != '/') {
        if (error) *error = "shm_name must start with '/'";
        return false;
    }
    if (error) error->clear();
    return true;
}

TransportEnvelope make_shared_frame_descriptor_envelope(const SharedFrameDescriptor& desc) {
    TransportEnvelope msg;
    msg.kind = TransportMessageKind::TEST;
    msg.seq = desc.seq;
    msg.timestamp_us = desc.timestamp_us;
    msg.width = desc.width;
    msg.height = desc.height;
    msg.channels = desc.bytes_per_pixel;
    msg.raw_size = static_cast<std::uint32_t>(std::min<std::uint64_t>(desc.payload_size, 0xFFFFFFFFULL));
    msg.payload = serialize_shared_frame_descriptor(desc);
    msg.payload_crc32 = compute_transport_crc(msg.payload);
    return msg;
}

bool parse_shared_frame_descriptor_envelope(const TransportEnvelope& msg,
                                            SharedFrameDescriptor& desc,
                                            std::string* error) {
    if (msg.kind != TransportMessageKind::TEST) {
        if (error) *error = "metadata envelope must use TransportMessageKind::TEST";
        return false;
    }
    const std::uint32_t crc = compute_transport_crc(msg.payload);
    if (crc != msg.payload_crc32) {
        if (error) *error = "metadata envelope payload crc mismatch";
        return false;
    }
    if (!deserialize_shared_frame_descriptor(msg.payload, desc, error)) {
        return false;
    }
    if (desc.seq != msg.seq) {
        if (error) *error = "descriptor seq does not match envelope seq";
        return false;
    }
    if (error) error->clear();
    return true;
}

SharedFrameDescriptor make_shared_frame_descriptor(const SensorFrame& frame,
                                                   const std::string& shm_name,
                                                   std::uint32_t buffer_index,
                                                   std::uint32_t buffer_count,
                                                   std::uint64_t buffer_size,
                                                   std::uint32_t alive_counter) {
    SharedFrameDescriptor desc;
    desc.seq = frame.frame_id;
    desc.timestamp_us = frame.timestamp_ns / 1000U;
    desc.width = frame.width;
    desc.height = frame.height;
    desc.format = frame.format;
    desc.bytes_per_pixel = (frame.width == 0U || frame.height == 0U)
                               ? 0U
                               : static_cast<std::uint32_t>(frame.data_size / (frame.width * frame.height));
    desc.payload_size = frame.data_size;
    desc.payload_crc32 = crc32_compute(frame.data.data(), frame.data.size());
    desc.buffer_index = buffer_index;
    desc.buffer_count = buffer_count;
    desc.buffer_size = buffer_size;
    desc.offset = static_cast<std::uint64_t>(buffer_index) * buffer_size;
    desc.alive_counter = alive_counter;
    std::memset(desc.shm_name, 0, sizeof(desc.shm_name));
    std::strncpy(desc.shm_name, shm_name.c_str(), sizeof(desc.shm_name) - 1U);
    return desc;
}

}  // namespace avm
