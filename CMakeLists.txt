cmake_minimum_required(VERSION 3.20)

project(Miliastra++ CXX)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING
    "Supported build configurations" FORCE
)

enable_testing()
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MILIASTRA_MSVC_STATIC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "${MILIASTRA_MSVC_STATIC_RUNTIME}")

# --------------------------------------------
# Offline third-party dependencies
# --------------------------------------------

set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_BUILD_TEST_HELPERS OFF CACHE BOOL "" FORCE)
set(ABSL_USE_EXTERNAL_GOOGLETEST OFF CACHE BOOL "" FORCE)
set(ABSL_USE_GOOGLETEST_HEAD OFF CACHE BOOL "" FORCE)
set(ABSL_BUILD_MONOLITHIC_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ABSL_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor/abseil-cpp
    ${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/abseil-cpp
    EXCLUDE_FROM_ALL
)

if(NOT TARGET absl::strings)
    message(FATAL_ERROR "The pinned local Abseil targets were not created")
endif()

set(protobuf_LOCAL_DEPENDENCIES_ONLY ON CACHE BOOL "" FORCE)
set(protobuf_FORCE_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOBUF_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBUPB OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor/protobuf
    ${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/protobuf
    EXCLUDE_FROM_ALL
)

if(NOT TARGET protobuf::libprotobuf OR NOT TARGET protobuf::libprotoc OR NOT TARGET protobuf::protoc)
    message(FATAL_ERROR "The pinned local Protobuf targets were not created")
endif()

set(MILIASTRA_THIRD_PARTY_RUNTIME_TARGETS
    libprotobuf
    libprotoc
    protoc
    utf8_validity
)

foreach(MILIASTRA_TARGET ${MILIASTRA_THIRD_PARTY_RUNTIME_TARGETS})
    if(TARGET ${MILIASTRA_TARGET})
        set_property(
            TARGET ${MILIASTRA_TARGET}
            PROPERTY MSVC_RUNTIME_LIBRARY "${MILIASTRA_MSVC_STATIC_RUNTIME}"
        )
    endif()
endforeach()

set(MILIASTRA_GIA_PROTO_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/Generated/GiaProto
)
set(MILIASTRA_GIA_PROTO
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor/genshin-ts/gia.proto
)
set(MILIASTRA_GIA_PROTO_HEADER
    ${MILIASTRA_GIA_PROTO_DIR}/gia.pb.h
)
set(MILIASTRA_GIA_PROTO_SOURCE
    ${MILIASTRA_GIA_PROTO_DIR}/gia.pb.cc
)

add_custom_command(
    OUTPUT
        ${MILIASTRA_GIA_PROTO_HEADER}
        ${MILIASTRA_GIA_PROTO_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MILIASTRA_GIA_PROTO_DIR}
    COMMAND $<TARGET_FILE:protobuf::protoc>
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor/genshin-ts
        --cpp_out=${MILIASTRA_GIA_PROTO_DIR}
        gia.proto
    DEPENDS
        protobuf::protoc
        ${MILIASTRA_GIA_PROTO}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor/genshin-ts
    VERBATIM
)

add_custom_target(MiliastraGiaProtoGeneration
    DEPENDS
        ${MILIASTRA_GIA_PROTO_HEADER}
        ${MILIASTRA_GIA_PROTO_SOURCE}
)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/Bin/$<CONFIG>-windows-x86_64/${PROJECT_NAME}
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/Bin/$<CONFIG>-windows-x86_64/${PROJECT_NAME}
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/Int/$<CONFIG>-windows-x86_64/${PROJECT_NAME}
)

# --------------------------------------------
# Gather source files
# --------------------------------------------

file(GLOB_RECURSE MILIASTRA_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/*.cpp
)

set(MILIASTRA_VENDOR_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor/nlohmann/json.hpp
)

set(MILIASTRA_PROJECT_FILES
    ${MILIASTRA_SOURCES}
    ${MILIASTRA_VENDOR_HEADERS}
    ${MILIASTRA_GIA_PROTO_SOURCE}
)

# --------------------------------------------
# Core Static Library
# --------------------------------------------

add_library(${PROJECT_NAME} STATIC
    ${MILIASTRA_PROJECT_FILES}
)

add_dependencies(${PROJECT_NAME} MiliastraGiaProtoGeneration)

target_link_libraries(${PROJECT_NAME} PRIVATE
    protobuf::libprotobuf
)

# --------------------------------------------
# Test Executables
# --------------------------------------------

set(TEST_TARGETS
    Phase1Tests
    Phase2Milestone1Tests
    Phase2Milestone2Tests
    Phase2Milestone3Tests
    Phase2Milestone4Tests
    Phase2Milestone5Tests
    Phase2Milestone6Tests
    Phase2Milestone7Tests
    Phase2Milestone8Tests
    Phase3Milestone1Tests
    Phase3Milestone2Tests
    Phase3Milestone3Tests
    Phase4Milestone1Tests
    Phase4Milestone2Tests
    Phase4Milestone3Tests
    Phase4Milestone4Tests
    Phase5Milestone1Tests
    Phase5Milestone2Tests
    Phase5Milestone3Tests
    Phase5Milestone4Tests
    Phase5Milestone5Tests
    Phase6Milestone1Tests
    Phase6FirstFixtureCoverageTests
    Phase6Milestone2Tests
    Phase6Milestone3Tests
    Phase6Milestone4Tests
)

foreach(TEST_NAME ${TEST_TARGETS})
    add_executable(${TEST_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/Tests/${TEST_NAME}.cpp
    )

    target_link_libraries(${TEST_NAME} PRIVATE
        ${PROJECT_NAME}
    )

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

target_link_libraries(Phase6Milestone4Tests PRIVATE
    protobuf::libprotobuf
)

target_compile_definitions(Phase6Milestone4Tests PRIVATE
    MILIASTRA_PHASE6_TEST_ACCESS
)

target_compile_definitions(Phase4Milestone2Tests PRIVATE
    MILIASTRA_PHASE4_TEST_ACCESS
)

target_compile_definitions(Phase4Milestone3Tests PRIVATE
    MILIASTRA_PHASE4_TEST_ACCESS
)

target_compile_definitions(Phase4Milestone4Tests PRIVATE
    MILIASTRA_PHASE4_TEST_ACCESS
)

# --------------------------------------------
# Include Directories
# --------------------------------------------

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Private
        ${MILIASTRA_GIA_PROTO_DIR}
)

# --------------------------------------------
# Compile Definitions
# --------------------------------------------

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS

    $<$<PLATFORM_ID:Windows>:MW_PLATFORM_WINDOWS>

    $<$<CONFIG:Debug>:MW_DEBUG>
    $<$<CONFIG:Release>:MW_RELEASE>
    $<$<CONFIG:Dist>:MW_DIST>
)

# --------------------------------------------
# MSVC Options
# --------------------------------------------

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /utf-8

        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Dist>:/O2>
    )
    set_property(
        TARGET ${PROJECT_NAME}
        PROPERTY VS_USE_MSVCCLMP true
    )
endif()

# --------------------------------------------
# Precompiled Header
# --------------------------------------------

target_precompile_headers(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/mpppch.h
)

# The P6.4 failure-injection and protobuf-inspection seam is compiled only into
# the dedicated test executable. The production archive keeps the same encoder
# source without test-only definitions or symbols.
add_library(Phase6Milestone4EncoderTestAccess OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Private/MiliastraPlusPlusGiaProtobufEncoder.cpp
)

target_include_directories(Phase6Milestone4EncoderTestAccess PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Private
    ${MILIASTRA_GIA_PROTO_DIR}
)

target_link_libraries(Phase6Milestone4EncoderTestAccess PRIVATE
    protobuf::libprotobuf
)

target_compile_definitions(Phase6Milestone4EncoderTestAccess PRIVATE
    MILIASTRA_PHASE6_TEST_ACCESS
    _CRT_SECURE_NO_WARNINGS

    $<$<PLATFORM_ID:Windows>:MW_PLATFORM_WINDOWS>

    $<$<CONFIG:Debug>:MW_DEBUG>
    $<$<CONFIG:Release>:MW_RELEASE>
    $<$<CONFIG:Dist>:MW_DIST>
)

if(MSVC)
    target_compile_options(Phase6Milestone4EncoderTestAccess PRIVATE
        /utf-8

        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Dist>:/O2>
    )
    set_property(
        TARGET Phase6Milestone4EncoderTestAccess
        PROPERTY VS_USE_MSVCCLMP true
    )
endif()

target_precompile_headers(Phase6Milestone4EncoderTestAccess PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/mpppch.h
)

target_sources(Phase6Milestone4Tests PRIVATE
    $<TARGET_OBJECTS:Phase6Milestone4EncoderTestAccess>
)
