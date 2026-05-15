cmake_minimum_required(VERSION 3.16)
project(autovision_mini_middleware_v1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic -O2)

include_directories(include)

add_library(avm_common
    src/crc32.cpp
    src/latency_profiler.cpp
    src/image_preprocess.cpp
    src/shared_status.cpp
    src/shm_frame_pool.cpp
    src/shm_ring_buffer.cpp
)

target_link_libraries(avm_common pthread rt)

add_executable(media_node
    src/media_node.cpp
    src/file_adapter.cpp
    src/lidar_sim_adapter.cpp
    src/camera_adapter_v4l2.cpp
)
target_link_libraries(media_node avm_common)

add_executable(preprocess_node src/preprocess_node.cpp)
target_link_libraries(preprocess_node avm_common)

add_executable(npu_stub_node
    src/npu_stub_node.cpp
    src/dummy_runtime.cpp
    src/rknn_runtime_stub.cpp
    src/runtime_factory.cpp
)
target_link_libraries(npu_stub_node avm_common)

add_executable(safety_monitor src/safety_monitor.cpp)
target_link_libraries(safety_monitor avm_common)

add_executable(control_service src/control_service.cpp)
target_link_libraries(control_service avm_common)

add_executable(control_client src/control_client.cpp)
target_link_libraries(control_client avm_common)