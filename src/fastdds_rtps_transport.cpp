/**
 * @file fastdds_rtps_transport.cpp
 * @brief Optional FastDDS/RTPS adapter implementation for AutoVision transport messages.
 */
#include "fastdds_rtps_transport.hpp"

#include "crc32.hpp"

#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>
#include <sstream>
#include <utility>

#ifdef AVM_HAS_FASTDDS
#include <fastrtps/attributes/TopicAttributes.h>
#include <fastrtps/qos/ReaderQos.h>
#include <fastrtps/qos/WriterQos.h>
#include <fastrtps/rtps/RTPSDomain.h>
#include <fastrtps/rtps/attributes/HistoryAttributes.h>
#include <fastrtps/rtps/attributes/ReaderAttributes.h>
#include <fastrtps/rtps/attributes/RTPSParticipantAttributes.h>
#include <fastrtps/rtps/attributes/WriterAttributes.h>
#include <fastrtps/rtps/common/CacheChange.h>
#include <fastrtps/rtps/common/Locator.h>
#include <fastrtps/rtps/common/WriteParams.h>
#include <fastrtps/rtps/history/ReaderHistory.h>
#include <fastrtps/rtps/history/WriterHistory.h>
#include <fastrtps/rtps/participant/RTPSParticipant.h>
#include <fastrtps/rtps/reader/ReaderListener.h>
#include <fastrtps/rtps/reader/RTPSReader.h>
#include <fastrtps/rtps/writer/RTPSWriter.h>
#endif

namespace avm {
namespace {
constexpr std::uint32_t kPacketMagic = 0x41565254U;  // "AVRT"
constexpr std::uint32_t kPacketVersion = 1U;
constexpr std::size_t kPacketHeaderBytes = 4U * sizeof(std::uint32_t) + 10U * sizeof(std::uint64_t);

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

[[maybe_unused]] TransportQosProfile qos_from_options(const FastddsRtpsOptions& options) {
    TransportQosProfile qos;
    qos.msg_size = options.max_payload_size;
    qos.depth = options.depth;
    qos.drop_oldest = true;
    qos.reliable = options.reliable;
    return qos;
}
}  // namespace

std::vector<std::uint8_t> serialize_transport_envelope(const TransportEnvelope& msg) {
    std::vector<std::uint8_t> out;
    out.reserve(kPacketHeaderBytes + msg.payload.size());
    append_u32(out, kPacketMagic);
    append_u32(out, kPacketVersion);
    append_u32(out, static_cast<std::uint32_t>(msg.kind));
    append_u32(out, msg.payload_crc32);
    append_u64(out, msg.seq);
    append_u64(out, msg.timestamp_us);
    append_u64(out, msg.width);
    append_u64(out, msg.height);
    append_u64(out, msg.channels);
    append_u64(out, msg.point_count);
    append_u64(out, msg.point_stride);
    append_u64(out, msg.raw_size);
    append_u64(out, msg.encoded_size);
    append_u64(out, static_cast<std::uint64_t>(msg.payload.size()));
    out.insert(out.end(), msg.payload.begin(), msg.payload.end());
    return out;
}

bool deserialize_transport_envelope(const std::vector<std::uint8_t>& bytes, TransportEnvelope& msg,
                                    std::string* error) {
    std::size_t off = 0;
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t kind = 0;
    std::uint64_t tmp = 0;
    if (!read_u32(bytes, off, magic) || magic != kPacketMagic) {
        if (error) *error = "bad packet magic";
        return false;
    }
    if (!read_u32(bytes, off, version) || version != kPacketVersion) {
        if (error) *error = "unsupported packet version";
        return false;
    }
    if (!read_u32(bytes, off, kind)) {
        if (error) *error = "missing kind";
        return false;
    }
    std::uint32_t crc = 0;
    if (!read_u32(bytes, off, crc)) {
        if (error) *error = "missing crc";
        return false;
    }
    msg = TransportEnvelope{};
    msg.kind = static_cast<TransportMessageKind>(kind);
    msg.payload_crc32 = crc;
    if (!read_u64(bytes, off, msg.seq) || !read_u64(bytes, off, msg.timestamp_us)) {
        if (error) *error = "missing sequence/timestamp";
        return false;
    }
    if (!read_u64(bytes, off, tmp)) { return false; }
    msg.width = static_cast<std::uint32_t>(tmp);
    if (!read_u64(bytes, off, tmp)) { return false; }
    msg.height = static_cast<std::uint32_t>(tmp);
    if (!read_u64(bytes, off, tmp)) { return false; }
    msg.channels = static_cast<std::uint32_t>(tmp);
    if (!read_u64(bytes, off, tmp)) { return false; }
    msg.point_count = static_cast<std::uint32_t>(tmp);
    if (!read_u64(bytes, off, tmp)) { return false; }
    msg.point_stride = static_cast<std::uint32_t>(tmp);
    if (!read_u64(bytes, off, tmp)) { return false; }
    msg.raw_size = static_cast<std::uint32_t>(tmp);
    if (!read_u64(bytes, off, tmp)) { return false; }
    msg.encoded_size = static_cast<std::uint32_t>(tmp);
    std::uint64_t payload_size = 0;
    if (!read_u64(bytes, off, payload_size)) {
        if (error) *error = "missing payload size";
        return false;
    }
    if (payload_size > static_cast<std::uint64_t>(bytes.size() - off)) {
        if (error) *error = "truncated payload";
        return false;
    }
    msg.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(off),
                       bytes.begin() + static_cast<std::ptrdiff_t>(off + payload_size));
    const auto computed = crc32_compute(msg.payload.data(), msg.payload.size());
    if (computed != msg.payload_crc32) {
        if (error) {
            std::ostringstream oss;
            oss << "payload crc mismatch expected=0x" << std::hex << msg.payload_crc32
                << " actual=0x" << computed;
            *error = oss.str();
        }
        return false;
    }
    if (error) error->clear();
    return true;
}

#ifdef AVM_HAS_FASTDDS
class FastddsRtpsBus::Impl {
public:
    explicit Impl(FastddsRtpsOptions options) : options_(std::move(options)) {}
    ~Impl() { close(); }

    bool open(std::string* error) {
        using namespace eprosima::fastrtps;
        using namespace eprosima::fastrtps::rtps;

        RTPSParticipantAttributes participant_attr;
        participant_attr.setName(options_.participant_name.c_str());
        participant_attr.builtin.use_WriterLivelinessProtocol = true;
        participant_attr.builtin.discovery_config.discoveryProtocol = DiscoveryProtocol::SIMPLE;
        participant_attr.builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;

        participant_ = RTPSDomain::createParticipant(options_.domain_id, participant_attr);
        if (participant_ == nullptr) {
            if (error) *error = "FastDDS createParticipant failed";
            return false;
        }

        TopicAttributes topic_attr;
        topic_attr.topicName = options_.topic;
        topic_attr.topicDataType = "avm_transport_envelope";
        topic_attr.topicKind = NO_KEY;
        topic_attr.historyQos.kind = KEEP_LAST_HISTORY_QOS;
        topic_attr.historyQos.depth = static_cast<int32_t>(std::max(1U, options_.depth));

        HistoryAttributes writer_hist_attr;
        writer_hist_attr.payloadMaxSize = static_cast<uint32_t>(options_.max_payload_size + kPacketHeaderBytes + 64U);
        writer_hist_attr.maximumReservedCaches = static_cast<int32_t>(std::max(1U, options_.depth));
        writer_history_ = new WriterHistory(writer_hist_attr);

        WriterAttributes writer_attr;
        writer_attr.endpoint.reliabilityKind = options_.reliable ? RELIABLE : BEST_EFFORT;
        writer_ = RTPSDomain::createRTPSWriter(participant_, writer_attr, writer_history_);
        if (writer_ == nullptr || !participant_->registerWriter(writer_, topic_attr, writer_qos_)) {
            if (error) *error = "FastDDS create/register writer failed";
            close();
            return false;
        }

        HistoryAttributes reader_hist_attr;
        reader_hist_attr.payloadMaxSize = writer_hist_attr.payloadMaxSize;
        reader_hist_attr.maximumReservedCaches = static_cast<int32_t>(std::max(1U, options_.depth));
        reader_history_ = new ReaderHistory(reader_hist_attr);

        ReaderAttributes reader_attr;
        reader_attr.endpoint.reliabilityKind = options_.reliable ? RELIABLE : BEST_EFFORT;
        reader_ = RTPSDomain::createRTPSReader(participant_, reader_attr, reader_history_, nullptr);
        if (reader_ == nullptr || !participant_->registerReader(reader_, topic_attr, reader_qos_)) {
            if (error) *error = "FastDDS create/register reader failed";
            close();
            return false;
        }
        opened_ = true;
        if (error) error->clear();
        return true;
    }

    bool publish(const TransportEnvelope& msg, std::string* error) {
        using namespace eprosima::fastrtps::rtps;
        if (!opened_ || writer_ == nullptr || writer_history_ == nullptr) {
            if (error) *error = "FastDDS bus is not open";
            return false;
        }
        const auto qos = qos_from_options(options_);
        const auto validation = validate_transport_message(msg, qos);
        if (!validation.ok) {
            stats_.size_errors += validation.size_errors;
            stats_.payload_errors += validation.payload_errors;
            if (error) *error = validation.reason;
            return false;
        }
        const auto bytes = serialize_transport_envelope(msg);
        CacheChange_t* change = writer_->new_change([&bytes]() -> uint32_t {
            return static_cast<uint32_t>(bytes.size());
        }, ALIVE);
        if (change == nullptr || change->serializedPayload.max_size < bytes.size()) {
            ++stats_.dropped;
            if (error) *error = "FastDDS new_change failed or payload too large";
            return false;
        }
        std::memcpy(change->serializedPayload.data, bytes.data(), bytes.size());
        change->serializedPayload.length = static_cast<uint32_t>(bytes.size());
        WriteParams params;
        params.related_sample_identity().sequence_number().high = static_cast<int32_t>((msg.seq >> 32U) & 0xFFFFFFFFU);
        params.related_sample_identity().sequence_number().low = static_cast<int32_t>(msg.seq & 0xFFFFFFFFU);
        bool ok = writer_history_->add_change(change, params);
        if (!ok) {
            writer_->remove_older_changes(static_cast<uint32_t>(std::max<std::uint32_t>(1, options_.depth / 2U)));
            ok = writer_history_->add_change(change, params);
        }
        if (!ok) {
            ++stats_.dropped;
            if (error) *error = "FastDDS WriterHistory add_change failed";
            return false;
        }
        ++stats_.sent;
        if (error) error->clear();
        return true;
    }

    bool take(TransportEnvelope& msg, std::string* error) {
        using namespace eprosima::fastrtps::rtps;
        if (!opened_ || reader_history_ == nullptr) {
            if (error) *error = "FastDDS bus is not open";
            return false;
        }
        CacheChange_t* change = nullptr;
        if (!reader_history_->get_min_change(&change) || change == nullptr) {
            ++stats_.empty_polls;
            if (error) *error = "empty";
            return false;
        }
        std::vector<std::uint8_t> bytes(change->serializedPayload.data,
                                       change->serializedPayload.data + change->serializedPayload.length);
        const bool ok = deserialize_transport_envelope(bytes, msg, error);
        reader_history_->remove_change(change);
        if (!ok) {
            ++stats_.payload_errors;
            return false;
        }
        const auto validation = validate_transport_message(msg, qos_from_options(options_));
        stats_.size_errors += validation.size_errors;
        stats_.payload_errors += validation.payload_errors;
        ++stats_.received;
        return validation.ok;
    }

    void close() {
        using namespace eprosima::fastrtps::rtps;
        opened_ = false;
        if (participant_ != nullptr) {
            if (writer_ != nullptr) {
                RTPSDomain::removeRTPSWriter(writer_);
                writer_ = nullptr;
            }
            if (reader_ != nullptr) {
                RTPSDomain::removeRTPSReader(reader_);
                reader_ = nullptr;
            }
            RTPSDomain::removeRTPSParticipant(participant_);
            participant_ = nullptr;
        }
        writer_history_ = nullptr;
        reader_history_ = nullptr;
    }

    FastddsRtpsStats stats() const { return stats_; }

private:
    FastddsRtpsOptions options_;
    bool opened_ = false;
    FastddsRtpsStats stats_{};
    eprosima::fastrtps::rtps::RTPSParticipant* participant_ = nullptr;
    eprosima::fastrtps::rtps::RTPSWriter* writer_ = nullptr;
    eprosima::fastrtps::rtps::RTPSReader* reader_ = nullptr;
    eprosima::fastrtps::rtps::WriterHistory* writer_history_ = nullptr;
    eprosima::fastrtps::rtps::ReaderHistory* reader_history_ = nullptr;
    eprosima::fastrtps::WriterQos writer_qos_;
    eprosima::fastrtps::ReaderQos reader_qos_;
};

bool fastdds_support_compiled() { return true; }
std::string fastdds_support_status() { return "compiled_with_fastrtps"; }
#else
class FastddsRtpsBus::Impl {
public:
    explicit Impl(FastddsRtpsOptions options) : options_(std::move(options)) {}
    bool open(std::string* error) {
        if (error) {
            *error = "FastDDS support is not compiled. Rebuild with -DAVM_ENABLE_FASTDDS=ON after installing FastDDS/Fast-RTPS 2.12.x.";
        }
        return false;
    }
    bool publish(const TransportEnvelope&, std::string* error) {
        if (error) *error = "FastDDS support is not compiled";
        return false;
    }
    bool take(TransportEnvelope&, std::string* error) {
        if (error) *error = "FastDDS support is not compiled";
        return false;
    }
    void close() {}
    FastddsRtpsStats stats() const { return stats_; }
private:
    FastddsRtpsOptions options_;
    FastddsRtpsStats stats_{};
};

bool fastdds_support_compiled() { return false; }
std::string fastdds_support_status() {
    return "not_compiled: install FastDDS/Fast-RTPS 2.12.x and rebuild with -DAVM_ENABLE_FASTDDS=ON";
}
#endif

FastddsRtpsBus::FastddsRtpsBus(FastddsRtpsOptions options)
    : options_(std::move(options)), impl_(std::make_unique<Impl>(options_)) {}

FastddsRtpsBus::~FastddsRtpsBus() = default;

bool FastddsRtpsBus::open(std::string* error) { return impl_->open(error); }
bool FastddsRtpsBus::publish(const TransportEnvelope& msg, std::string* error) { return impl_->publish(msg, error); }
bool FastddsRtpsBus::take(TransportEnvelope& msg, std::string* error) { return impl_->take(msg, error); }
void FastddsRtpsBus::close() { impl_->close(); }
FastddsRtpsStats FastddsRtpsBus::stats() const { return impl_->stats(); }

}  // namespace avm
