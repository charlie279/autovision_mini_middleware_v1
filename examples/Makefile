# examples/Makefile
#
# 功能：
#   统一编译和运行 examples/cpp/ 下的 C++ 示例程序。
#   每个示例用于单独理解一个模块的接口调用方式。
#
# 用法：
#   make list
#   make run EXAMPLE=00_crc32
#   make run EXAMPLE=03_file_adapter
#   make all
#   make run_frame_ipc
#   make clean

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -I../include
LDFLAGS := -pthread -lrt

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
	10_control_query

SRCS_00_crc32 := \
	$(CPP_DIR)/00_crc32.cpp \
	../src/crc32.cpp

SRCS_01_latency_profiler := \
	$(CPP_DIR)/01_latency_profiler.cpp \
	../src/latency_profiler.cpp

SRCS_02_control_cmd := \
	$(CPP_DIR)/02_control_cmd.cpp

SRCS_03_file_adapter := \
	$(CPP_DIR)/03_file_adapter.cpp \
	../src/file_adapter.cpp

SRCS_04_lidar_sim_adapter := \
	$(CPP_DIR)/04_lidar_sim_adapter.cpp \
	../src/lidar_sim_adapter.cpp

SRCS_05_shm_frame_pool := \
	$(CPP_DIR)/05_shm_frame_pool.cpp \
	../src/shm_frame_pool.cpp

SRCS_06_shm_ring_buffer := \
	$(CPP_DIR)/06_shm_ring_buffer.cpp \
	../src/shm_ring_buffer.cpp

SRCS_07_shared_status := \
	$(CPP_DIR)/07_shared_status.cpp \
	../src/shared_status.cpp

SRCS_09_sensor_to_shm := \
	$(CPP_DIR)/09_sensor_to_shm.cpp \
	../src/file_adapter.cpp \
	../src/shm_frame_pool.cpp \
	../src/shm_ring_buffer.cpp \
	../src/crc32.cpp

SRCS_10_control_query := \
	$(CPP_DIR)/10_control_query.cpp

PIPELINE_SRCS_PRODUCER := \
	$(CPP_DIR)/08_frame_producer.cpp \
	../src/shm_frame_pool.cpp \
	../src/shm_ring_buffer.cpp \
	../src/crc32.cpp

PIPELINE_SRCS_CONSUMER := \
	$(CPP_DIR)/08_frame_consumer.cpp \
	../src/shm_frame_pool.cpp \
	../src/shm_ring_buffer.cpp \
	../src/crc32.cpp

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
	rm -f /dev/shm/avm_example_frame_pool \
	      /dev/shm/avm_example_frame_meta_ring \
	      /dev/shm/avm_example_status \
	      /dev/shm/avm_example_sensor_pool \
	      /dev/shm/avm_example_sensor_ring
