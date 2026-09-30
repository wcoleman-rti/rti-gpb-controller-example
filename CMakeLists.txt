cmake_minimum_required(VERSION 3.18)
project(gpb_controller_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    rticonnextdds-cmake-utils
    GIT_REPOSITORY https://github.com/rticommunity/rticonnextdds-cmake-utils.git
    GIT_TAG main
)
FetchContent_MakeAvailable(rticonnextdds-cmake-utils)
list(APPEND CMAKE_MODULE_PATH "${rticonnextdds-cmake-utils_SOURCE_DIR}/cmake/Modules")
find_package(RTIConnextDDS 7.7.0 REQUIRED)

set(RTI_PROTOBUF_PREFIX
    "${CONNEXTDDS_DIR}/third_party/protobuf-33.0/${CONNEXTDDS_ARCH}/release")
set(RTI_PROTOBUF_CMAKE_DIR "${RTI_PROTOBUF_PREFIX}/lib/cmake/protobuf")
if(NOT Protobuf_DIR AND EXISTS "${RTI_PROTOBUF_CMAKE_DIR}/protobuf-config.cmake")
    set(Protobuf_DIR "${RTI_PROTOBUF_CMAKE_DIR}" CACHE PATH "RTI bundled Protobuf package")
endif()
list(PREPEND CMAKE_PREFIX_PATH "${RTI_PROTOBUF_PREFIX}")
find_package(Protobuf CONFIG REQUIRED)

find_program(PROTOC_EXECUTABLE NAMES protoc HINTS "${RTI_PROTOBUF_PREFIX}/bin" REQUIRED)
include(ConnextDdsCodegen)

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(MODEL_NAME vehicle_model)

add_custom_command(
    OUTPUT
        "${GENERATED_DIR}/${MODEL_NAME}.idl"
        "${GENERATED_DIR}/${MODEL_NAME}.pb.cc"
        "${GENERATED_DIR}/${MODEL_NAME}.pb.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIR}"
    COMMAND "${PROTOC_EXECUTABLE}"
        "--cpp_out=${GENERATED_DIR}"
        "--connext-cpp_out=${GENERATED_DIR}"
        "--idl4_out=${GENERATED_DIR}"
        "--plugin=protoc-gen-connext-cpp=${CONNEXTDDS_DIR}/bin/protoc-gen-connext-cpp"
        "--plugin=protoc-gen-idl4=${CONNEXTDDS_DIR}/bin/protoc-gen-idl4"
        "-I${PROTO_DIR}"
        "${PROTO_DIR}/${MODEL_NAME}.proto"
    DEPENDS "${PROTO_DIR}/${MODEL_NAME}.proto"
    VERBATIM
)

connextdds_rtiddsgen_run(
    IDL_FILE "${GENERATED_DIR}/${MODEL_NAME}.idl"
    OUTPUT_DIRECTORY "${GENERATED_DIR}"
    LANG C++11
    VAR ${MODEL_NAME}
    INCLUDE_DIRS "${GENERATED_DIR}"
    EXTRA_ARGS -standard PROTOBUF_CPP
)

add_library(vehicle_types STATIC
    ${vehicle_model_CXX11_SOURCES}
    ${vehicle_model_CXX11_HEADERS}
    "${GENERATED_DIR}/${MODEL_NAME}.pb.cc"
    "${GENERATED_DIR}/${MODEL_NAME}.pb.h"
)
target_include_directories(vehicle_types PUBLIC "${GENERATED_DIR}")
target_link_libraries(vehicle_types PUBLIC protobuf::libprotobuf RTIConnextDDS::cpp2_api)

add_executable(vehicle_controller src/vehicle_controller.cpp)
target_link_libraries(vehicle_controller PRIVATE vehicle_types)

add_executable(vehicle_simulator src/vehicle_simulator.cpp)
target_link_libraries(vehicle_simulator PRIVATE vehicle_types)