# Minimum CMake required
cmake_minimum_required(VERSION 3.16)

project(replicant CXX)
set(CMAKE_CXX_STANDARD 14)

find_program(CCACHE ccache)
if(CCACHE)
  message("using ccache")
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
else()
  message("ccache not found")
endif()

# Protobuf
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)

# gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Protobuf-compiler
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)


# Proto file
get_filename_component(proto "multipaxos.proto" ABSOLUTE)
get_filename_component(proto_path "${proto}" PATH)

# Generated sources
set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/multipaxos.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/multipaxos.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/multipaxos.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/multipaxos.grpc.pb.h")
add_custom_command(
      OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${proto}"
      DEPENDS "${proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("/opt/xilinx/xrt/include/")

link_directories("/opt/xilinx/xrt/lib/")

add_library(grpc_proto
  ${grpc_srcs}
  ${grpc_hdrs}
  ${proto_srcs}
  ${proto_hdrs})
target_link_libraries(grpc_proto
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-error=pedantic -Wno-error=unused-parameter)
add_executable(replicant replicant.cc multipaxos.cc client_manager.cc client.cc
               log.cc xrt_log.cc common_log.cc kvstore.cc memkvstore.cc main.cc)

find_package(gflags REQUIRED)

target_link_libraries(replicant
    gflags
    ${_REFLECTION}
    grpc_proto
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    "xrt_coreutil")
