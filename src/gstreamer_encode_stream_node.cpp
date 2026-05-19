/**
 * @file gstreamer_encode_stream_node.cpp
 * @brief V2.6 GStreamer encode/stream demo: SensorFrame/appsrc -> encoder/filesink or custom pipeline.
 */
#include "gstreamer_camera_adapter.hpp"
#include "gstreamer_pipeline_config.hpp"
#include "sensor_frame.hpp"
#include "time_utils.hpp"

#include <chrono>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#ifdef AVM_HAS_GSTREAMER
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#endif

namespace {
std::string arg_value(int argc, char** argv, const std::string& key, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return default_value;
}

bool has_flag(int argc, char** argv, const std::string& key) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == key) {
            return true;
        }
    }
    return false;
}

void ensure_parent_dir(const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

SensorFrame make_synthetic_frame(std::uint64_t seq, std::uint32_t width, std::uint32_t height, const std::string& format) {
    const auto normalized = avm::gst_normalized_format(format);
    SensorFrame frame;
    frame.frame_id = seq;
    frame.timestamp_ns = avm::now_ns();
    frame.sensor_type = 0;
    frame.width = width;
    frame.height = height;
    frame.format = avm::gst_avm_format_code(normalized);
    frame.data_size = static_cast<std::uint32_t>(avm::gst_frame_payload_size(width, height, normalized));
    frame.stride_bytes = height == 0 ? 0 : frame.data_size / height;
    frame.data.resize(frame.data_size);
    const std::uint8_t salt = normalized == "rgb" ? 0x52U : (normalized == "nv12" ? 0x4EU : 0x59U);
    for (std::size_t i = 0; i < frame.data.size(); ++i) {
        frame.data[i] = static_cast<std::uint8_t>((i * 41U + seq * 13U + salt) & 0xFFU);
    }
    return frame;
}

void write_header(std::ofstream& csv) {
    csv << "role,compiled,source,device,format,width,height,fps,frames_requested,pushed,output,output_bytes,status\n";
}

#ifdef AVM_HAS_GSTREAMER
void ensure_gst_initialized() {
    static bool initialized = false;
    if (!initialized) {
        int argc = 0;
        char** argv = nullptr;
        gst_init(&argc, &argv);
        initialized = true;
    }
}
#endif
}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    const std::string source = arg_value(argc, argv, "--source", "synthetic");
    const std::string device = arg_value(argc, argv, "--device", "/dev/video0");
    const auto width = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--width", "640")));
    const auto height = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--height", "480")));
    const auto fps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--fps", "30")));
    const auto frames = static_cast<std::uint64_t>(std::stoull(arg_value(argc, argv, "--frames", "120")));
    const std::string format = avm::gst_normalized_format(arg_value(argc, argv, "--format", "yuyv"));
    const std::string codec = arg_value(argc, argv, "--codec", "h264");
    const std::string output_file = arg_value(argc, argv, "--encoded-output", codec == "raw" ? "logs/gstreamer_output.raw" : (codec == "h265" ? "logs/gstreamer_output.h265" : "logs/gstreamer_output.h264"));
    const auto bitrate_kbps = static_cast<std::uint32_t>(std::stoul(arg_value(argc, argv, "--bitrate-kbps", "4000")));
    const std::string custom_pipeline = arg_value(argc, argv, "--pipeline", "");
    const std::string csv_path = arg_value(argc, argv, "--output", "logs/benchmark_v2_6_gstreamer_camera/gstreamer_encode_stream.csv");
    const bool use_dmabuf = has_flag(argc, argv, "--use-dmabuf");
    const bool allow_unavailable = has_flag(argc, argv, "--allow-unavailable");

    ensure_parent_dir(csv_path);
    ensure_parent_dir(output_file);
    std::ofstream csv(csv_path);
    write_header(csv);

    if (!GStreamerCameraAdapter::compiled()) {
        std::cout << "[gstreamer_encode_stream_node] status=" << GStreamerCameraAdapter::support_status() << "\n";
        csv << "encode,false," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
            << "," << frames << ",0," << output_file << ",0,NOT_COMPILED\n";
        return allow_unavailable ? 0 : 2;
    }

#ifndef AVM_HAS_GSTREAMER
    (void)source; (void)device; (void)width; (void)height; (void)fps; (void)frames; (void)format; (void)codec;
    (void)output_file; (void)bitrate_kbps; (void)custom_pipeline; (void)use_dmabuf;
    return allow_unavailable ? 0 : 2;
#else
    ensure_gst_initialized();

    avm::GStreamerEncodePipelineConfig enc_cfg;
    enc_cfg.output = output_file;
    enc_cfg.width = width;
    enc_cfg.height = height;
    enc_cfg.fps = fps;
    enc_cfg.format = format;
    enc_cfg.codec = codec;
    enc_cfg.bitrate_kbps = bitrate_kbps;
    const std::string pipeline_text = custom_pipeline.empty() ? avm::build_gstreamer_appsrc_encode_pipeline(enc_cfg) : custom_pipeline;
    if (!avm::gstreamer_pipeline_has_appsrc(pipeline_text, "appsrc")) {
        std::cerr << "[gstreamer_encode_stream_node] pipeline must contain appsrc name=appsrc\n";
        csv << "encode,true," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
            << "," << frames << ",0," << output_file << ",0,INVALID_PIPELINE\n";
        return 2;
    }

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(pipeline_text.c_str(), &error);
    if (!pipeline) {
        std::cerr << "[gstreamer_encode_stream_node] gst_parse_launch failed: " << (error ? error->message : "unknown") << "\n";
        if (error) {
            g_error_free(error);
        }
        csv << "encode,true," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
            << "," << frames << ",0," << output_file << ",0,PIPELINE_OPEN_FAILED\n";
        return 3;
    }
    if (error) {
        g_error_free(error);
    }

    GstElement* appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "appsrc");
    if (!appsrc) {
        gst_object_unref(pipeline);
        csv << "encode,true," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
            << "," << frames << ",0," << output_file << ",0,APPSRC_NOT_FOUND\n";
        return 4;
    }

    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, avm::gst_caps_format(format).c_str(),
                                        "width", G_TYPE_INT, static_cast<int>(width),
                                        "height", G_TYPE_INT, static_cast<int>(height),
                                        "framerate", GST_TYPE_FRACTION, static_cast<int>(fps), 1,
                                        nullptr);
    g_object_set(G_OBJECT(appsrc), "caps", caps, "is-live", TRUE, "format", GST_FORMAT_TIME, "do-timestamp", TRUE, "block", TRUE, nullptr);
    gst_caps_unref(caps);

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[gstreamer_encode_stream_node] set PLAYING failed\n";
        gst_object_unref(appsrc);
        gst_object_unref(pipeline);
        csv << "encode,true," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
            << "," << frames << ",0," << output_file << ",0,PLAYING_FAILED\n";
        return 5;
    }

    std::unique_ptr<SensorAdapter> camera;
    if (source == "gstreamer") {
        auto gst_cam = std::make_unique<GStreamerCameraAdapter>(device, width, height, fps, format, use_dmabuf);
        if (!gst_cam->open() || !gst_cam->start()) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(appsrc);
            gst_object_unref(pipeline);
            csv << "encode,true," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
                << "," << frames << ",0," << output_file << ",0,CAMERA_OPEN_FAILED\n";
            return 6;
        }
        camera = std::move(gst_cam);
    } else if (source != "synthetic") {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(appsrc);
        gst_object_unref(pipeline);
        csv << "encode,true," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
            << "," << frames << ",0," << output_file << ",0,UNSUPPORTED_SOURCE\n";
        return 2;
    }

    std::uint64_t pushed = 0;
    const auto frame_duration = fps == 0 ? 0ULL : static_cast<unsigned long long>(GST_SECOND / fps);
    for (std::uint64_t i = 0; i < frames; ++i) {
        SensorFrame frame;
        if (camera) {
            if (!camera->readFrame(frame)) {
                std::cerr << "[gstreamer_encode_stream_node] camera read failed index=" << i << "\n";
                break;
            }
        } else {
            frame = make_synthetic_frame(i + 1U, width, height, format);
            if (fps > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1000000U / fps));
            }
        }

        GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame.data.size(), nullptr);
        GstMapInfo map{};
        if (!buffer || !gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            if (buffer) {
                gst_buffer_unref(buffer);
            }
            break;
        }
        std::memcpy(map.data, frame.data.data(), frame.data.size());
        gst_buffer_unmap(buffer, &map);
        GST_BUFFER_PTS(buffer) = static_cast<GstClockTime>(i * frame_duration);
        GST_BUFFER_DURATION(buffer) = static_cast<GstClockTime>(frame_duration);

        GstFlowReturn flow = GST_FLOW_OK;
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &flow);
        gst_buffer_unref(buffer);
        if (flow != GST_FLOW_OK) {
            std::cerr << "[gstreamer_encode_stream_node] push-buffer flow=" << flow << "\n";
            break;
        }
        ++pushed;
    }

    GstFlowReturn eos_ret = GST_FLOW_OK;
    g_signal_emit_by_name(appsrc, "end-of-stream", &eos_ret);
    GstBus* bus = gst_element_get_bus(pipeline);
    if (bus) {
        GstMessage* msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        if (msg) {
            if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                GError* err = nullptr;
                gchar* debug = nullptr;
                gst_message_parse_error(msg, &err, &debug);
                std::cerr << "[gstreamer_encode_stream_node] pipeline error: " << (err ? err->message : "unknown") << "\n";
                if (err) {
                    g_error_free(err);
                }
                if (debug) {
                    g_free(debug);
                }
            }
            gst_message_unref(msg);
        }
        gst_object_unref(bus);
    }

    if (camera) {
        camera->stop();
    }
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsrc);
    gst_object_unref(pipeline);

    const std::uintmax_t output_bytes = std::filesystem::exists(output_file) ? std::filesystem::file_size(output_file) : 0U;
    const bool pass = pushed == frames && output_bytes > 0U;
    csv << "encode,true," << source << "," << device << "," << format << "," << width << "," << height << "," << fps
        << "," << frames << "," << pushed << "," << output_file << "," << output_bytes << "," << (pass ? "PASS" : "PARTIAL") << "\n";
    std::cout << "[gstreamer_encode_stream_node] pipeline=" << pipeline_text << "\n";
    std::cout << "[gstreamer_encode_stream_node] pushed=" << pushed << " output=" << output_file
              << " output_bytes=" << output_bytes << " result=" << (pass ? "PASS" : "PARTIAL")
              << " csv=" << csv_path << "\n";
    return pass ? 0 : 1;
#endif
}
