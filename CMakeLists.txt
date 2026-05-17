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
    src/frame_lease.cpp
    src/record_replay.cpp
    src/timestamp_sync.cpp
    src/runtime_metadata.cpp
    src/detection_result.cpp
    src/fd_passing.cpp
    src/system_tuning.cpp
    src/shared_status.cpp
    src/shm_frame_pool.cpp
    src/shm_ring_buffer.cpp
)

target_link_libraries(avm_common pthread rt)

add_executable(media_node
    src/media_node.cpp
    src/file_adapter.cpp
    src/lidar_sim_adapter.cpp
    src/udp_lidar_sim_adapter.cpp
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

add_executable(algorithm_stub_node src/algorithm_stub_node.cpp)
target_link_libraries(algorithm_stub_node avm_common)

add_executable(safety_monitor src/safety_monitor.cpp)
target_link_libraries(safety_monitor avm_common)

add_executable(control_service src/control_service.cpp)
target_link_libraries(control_service avm_common)

add_executable(control_client src/control_client.cpp)
target_link_libraries(control_client avm_common)


# ============================================================
# V2.1/V2.2/V2.3 transport bridge / Transport pattern validation
# ============================================================
option(AVM_ENABLE_FASTDDS "Enable real FastDDS/Fast-RTPS 2.12.x adapter" OFF)

add_library(avm_transport
    src/transport_message.cpp
    src/local_pubsub_transport.cpp
    src/transport_endpoint.cpp
    src/fastdds_rtps_transport.cpp
)
target_link_libraries(avm_transport avm_common pthread)

if(AVM_ENABLE_FASTDDS)
    find_package(fastrtps CONFIG REQUIRED)
    find_package(fastcdr CONFIG REQUIRED)
    target_compile_definitions(avm_transport PUBLIC AVM_HAS_FASTDDS)
    target_link_libraries(avm_transport fastrtps fastcdr)
endif()

add_executable(transport_four_links_demo src/transport_four_links_demo.cpp)
target_link_libraries(transport_four_links_demo avm_transport avm_common)

add_executable(transport_pattern_demo src/transport_pattern_demo.cpp)
target_link_libraries(transport_pattern_demo avm_transport avm_common)

add_executable(fastdds_rtps_loopback_demo src/fastdds_rtps_loopback_demo.cpp)
target_link_libraries(fastdds_rtps_loopback_demo avm_transport avm_common)

add_executable(camera_fastdds_pub
    src/camera_fastdds_pub.cpp
    src/camera_adapter_v4l2.cpp
)
target_link_libraries(camera_fastdds_pub avm_transport avm_common)

add_executable(camera_fastdds_sub src/camera_fastdds_sub.cpp)
target_link_libraries(camera_fastdds_sub avm_transport avm_common)

# ============================================================
# V2.0 video encode module
# ============================================================
option(AVM_HAS_FFMPEG  "Enable FFmpeg soft video encoder"        OFF)
option(AVM_HAS_MPP     "Enable Rockchip MPP hardware encoder"    OFF)
option(AVM_HAS_V4L2M2M "Enable V4L2 M2M hardware encoder"        ON)

if(AVM_HAS_FFMPEG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libswscale)
    add_compile_definitions(AVM_HAS_FFMPEG)
endif()

if(AVM_HAS_MPP)
    find_library(MPP_LIB rockchip_mpp REQUIRED)
    add_compile_definitions(AVM_HAS_MPP)
endif()

if(AVM_HAS_V4L2M2M)
    add_compile_definitions(AVM_HAS_V4L2M2M)
endif()

add_library(avm_encoder
    src/soft_video_encoder.cpp
    src/v4l2_m2m_encoder.cpp
    src/mpp_video_encoder.cpp
    src/video_encoder_factory.cpp
)
target_link_libraries(avm_encoder avm_common)
if(AVM_HAS_FFMPEG)
    target_include_directories(avm_encoder PUBLIC ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(avm_encoder ${FFMPEG_LIBRARIES})
endif()
if(AVM_HAS_MPP)
    target_link_libraries(avm_encoder ${MPP_LIB})
endif()

add_executable(encode_sink_node src/encode_sink_node.cpp)
target_link_libraries(encode_sink_node avm_encoder avm_common)

