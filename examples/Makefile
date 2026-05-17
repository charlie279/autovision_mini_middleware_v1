# examples/Makefile
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -I../include
LDFLAGS := -pthread -lrt

FASTDDS_PREFIX ?= $(HOME)/Fast-DDS/install
FASTDDS_LIB_DIR := $(FASTDDS_PREFIX)/lib
FASTDDS_INC_DIR := $(FASTDDS_PREFIX)/include
FASTDDS_AVAILABLE := $(wildcard $(FASTDDS_LIB_DIR)/libfastrtps.so*)

ifneq ($(FASTDDS_AVAILABLE),)
EXAMPLE23_FASTDDS_CXXFLAGS := -DAVM_HAS_FASTDDS -I$(FASTDDS_INC_DIR)
EXAMPLE23_FASTDDS_LDFLAGS := -L$(FASTDDS_LIB_DIR) -Wl,-rpath,$(FASTDDS_LIB_DIR) -lfastrtps -lfastcdr -lssl -lcrypto
else
EXAMPLE23_FASTDDS_CXXFLAGS :=
EXAMPLE23_FASTDDS_LDFLAGS :=
endif

BIN_DIR := bin
CPP_DIR := cpp
EXAMPLE ?= 00_crc32

EXAMPLES := \
	00_crc32 \
	01_latency_profiler \
	02_control_cmd \
	03_file_adapter \
	04_lidar_sim_adapter \
	05_shm_frame_pool \
	06_shm_ring_buffer \
	07_shared_status \
	09_sensor_to_shm \
	10_control_query \
	11_camera_adapter_v4l2 \
	12_yuyv_fused_preprocess \
	13_preprocess_benchmark_compare \
	14_frame_lease_pool \
	15_fd_passing_alive_counter \
	16_udp_lidar_timestamp_sync \
	17_nv12_libyuv_preprocess \
	18_runtime_metadata_detection \
	19_affinity_priority_benchmark \
	20_video_encode_benchmark \
	21_transport_four_links \
	22_transport_pattern \
	23_fastdds_packet_codec

SRCS_00_crc32 := $(CPP_DIR)/00_crc32.cpp ../src/crc32.cpp
SRCS_01_latency_profiler := $(CPP_DIR)/01_latency_profiler.cpp ../src/latency_profiler.cpp
SRCS_02_control_cmd := $(CPP_DIR)/02_control_cmd.cpp
SRCS_03_file_adapter := $(CPP_DIR)/03_file_adapter.cpp ../src/file_adapter.cpp
SRCS_04_lidar_sim_adapter := $(CPP_DIR)/04_lidar_sim_adapter.cpp ../src/lidar_sim_adapter.cpp
SRCS_05_shm_frame_pool := $(CPP_DIR)/05_shm_frame_pool.cpp ../src/shm_frame_pool.cpp
SRCS_06_shm_ring_buffer := $(CPP_DIR)/06_shm_ring_buffer.cpp ../src/shm_ring_buffer.cpp
SRCS_07_shared_status := $(CPP_DIR)/07_shared_status.cpp ../src/shared_status.cpp
SRCS_09_sensor_to_shm := $(CPP_DIR)/09_sensor_to_shm.cpp ../src/file_adapter.cpp ../src/shm_frame_pool.cpp ../src/shm_ring_buffer.cpp ../src/crc32.cpp
SRCS_10_control_query := $(CPP_DIR)/10_control_query.cpp
SRCS_11_camera_adapter_v4l2 := $(CPP_DIR)/11_camera_adapter_v4l2.cpp ../src/camera_adapter_v4l2.cpp
SRCS_12_yuyv_fused_preprocess := $(CPP_DIR)/12_yuyv_fused_preprocess.cpp ../src/image_preprocess.cpp ../src/latency_profiler.cpp
SRCS_13_preprocess_benchmark_compare := $(CPP_DIR)/13_preprocess_benchmark_compare.cpp ../src/image_preprocess.cpp ../src/latency_profiler.cpp
SRCS_14_frame_lease_pool := $(CPP_DIR)/14_frame_lease_pool.cpp ../src/frame_lease.cpp
SRCS_15_fd_passing_alive_counter := $(CPP_DIR)/15_fd_passing_alive_counter.cpp ../src/fd_passing.cpp
SRCS_16_udp_lidar_timestamp_sync := $(CPP_DIR)/16_udp_lidar_timestamp_sync.cpp ../src/udp_lidar_sim_adapter.cpp ../src/timestamp_sync.cpp
SRCS_17_nv12_libyuv_preprocess := $(CPP_DIR)/17_nv12_libyuv_preprocess.cpp ../src/image_preprocess.cpp ../src/latency_profiler.cpp
SRCS_18_runtime_metadata_detection := $(CPP_DIR)/18_runtime_metadata_detection.cpp ../src/rknn_runtime_stub.cpp ../src/runtime_metadata.cpp ../src/detection_result.cpp
SRCS_19_affinity_priority_benchmark := $(CPP_DIR)/19_affinity_priority_benchmark.cpp ../src/system_tuning.cpp
SRCS_20_video_encode_benchmark := $(CPP_DIR)/20_video_encode_benchmark.cpp ../src/soft_video_encoder.cpp ../src/v4l2_m2m_encoder.cpp ../src/mpp_video_encoder.cpp ../src/video_encoder_factory.cpp ../src/latency_profiler.cpp
SRCS_21_transport_four_links := $(CPP_DIR)/21_transport_four_links.cpp ../src/transport_message.cpp ../src/local_pubsub_transport.cpp ../src/crc32.cpp
SRCS_22_transport_pattern := $(CPP_DIR)/22_transport_pattern.cpp ../src/transport_message.cpp ../src/local_pubsub_transport.cpp ../src/transport_endpoint.cpp ../src/fastdds_rtps_transport.cpp ../src/crc32.cpp
SRCS_23_fastdds_packet_codec := $(CPP_DIR)/23_fastdds_packet_codec.cpp ../src/fastdds_rtps_transport.cpp ../src/transport_message.cpp ../src/crc32.cpp

PIPELINE_SRCS_PRODUCER := $(CPP_DIR)/08_frame_producer.cpp ../src/shm_frame_pool.cpp ../src/shm_ring_buffer.cpp ../src/crc32.cpp
PIPELINE_SRCS_CONSUMER := $(CPP_DIR)/08_frame_consumer.cpp ../src/shm_frame_pool.cpp ../src/shm_ring_buffer.cpp ../src/crc32.cpp

.PHONY: all list run frame_ipc run_frame_ipc clean dirs prepare

all: dirs $(addprefix $(BIN_DIR)/,$(EXAMPLES)) frame_ipc

prepare:
	cd .. && ./scripts/prepare_input.sh

dirs:
	@mkdir -p $(BIN_DIR) logs

list:
	@echo "Available examples:"
	@for e in $(EXAMPLES); do echo "  $$e"; done
	@echo "  08_frame_ipc: build with make frame_ipc, run with make run_frame_ipc"

run: dirs $(BIN_DIR)/$(EXAMPLE)
	@echo "[examples] running $(EXAMPLE)"
	@cd .. && ./examples/$(BIN_DIR)/$(EXAMPLE)

$(BIN_DIR)/00_crc32: $(SRCS_00_crc32) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/01_latency_profiler: $(SRCS_01_latency_profiler) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/02_control_cmd: $(SRCS_02_control_cmd) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/03_file_adapter: $(SRCS_03_file_adapter) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/04_lidar_sim_adapter: $(SRCS_04_lidar_sim_adapter) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/05_shm_frame_pool: $(SRCS_05_shm_frame_pool) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/06_shm_ring_buffer: $(SRCS_06_shm_ring_buffer) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/07_shared_status: $(SRCS_07_shared_status) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/09_sensor_to_shm: $(SRCS_09_sensor_to_shm) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/10_control_query: $(SRCS_10_control_query) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/11_camera_adapter_v4l2: $(SRCS_11_camera_adapter_v4l2) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/12_yuyv_fused_preprocess: $(SRCS_12_yuyv_fused_preprocess) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/13_preprocess_benchmark_compare: $(SRCS_13_preprocess_benchmark_compare) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/14_frame_lease_pool: $(SRCS_14_frame_lease_pool) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/15_fd_passing_alive_counter: $(SRCS_15_fd_passing_alive_counter) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/16_udp_lidar_timestamp_sync: $(SRCS_16_udp_lidar_timestamp_sync) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/17_nv12_libyuv_preprocess: $(SRCS_17_nv12_libyuv_preprocess) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/18_runtime_metadata_detection: $(SRCS_18_runtime_metadata_detection) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/19_affinity_priority_benchmark: $(SRCS_19_affinity_priority_benchmark) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/20_video_encode_benchmark: $(SRCS_20_video_encode_benchmark) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/21_transport_four_links: $(SRCS_21_transport_four_links) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/22_transport_pattern: $(SRCS_22_transport_pattern) | dirs
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
$(BIN_DIR)/23_fastdds_packet_codec: $(SRCS_23_fastdds_packet_codec) | dirs
	$(CXX) $(CXXFLAGS) $(EXAMPLE23_FASTDDS_CXXFLAGS) $^ -o $@ $(EXAMPLE23_FASTDDS_LDFLAGS) $(LDFLAGS)

frame_ipc: dirs
	$(CXX) $(CXXFLAGS) $(PIPELINE_SRCS_PRODUCER) -o $(BIN_DIR)/08_frame_producer $(LDFLAGS)
	$(CXX) $(CXXFLAGS) $(PIPELINE_SRCS_CONSUMER) -o $(BIN_DIR)/08_frame_consumer $(LDFLAGS)

run_frame_ipc: frame_ipc
	@rm -f /dev/shm/avm_example_frame_pool /dev/shm/avm_example_frame_meta_ring
	@echo "[examples] starting consumer..."
	@./$(BIN_DIR)/08_frame_consumer > logs/08_frame_consumer.log 2>&1 & \
	PID=$$!; \
	sleep 0.2; \
	echo "[examples] starting producer..."; \
	./$(BIN_DIR)/08_frame_producer > logs/08_frame_producer.log 2>&1; \
	wait $$PID; \
	echo "[examples] producer log:"; \
	cat logs/08_frame_producer.log; \
	echo "[examples] consumer log:"; \
	cat logs/08_frame_consumer.log

clean:
	rm -rf $(BIN_DIR) logs
	rm -f /dev/shm/avm_example_frame_pool /dev/shm/avm_example_frame_meta_ring /dev/shm/avm_example_status /dev/shm/avm_example_sensor_pool /dev/shm/avm_example_sensor_ring
