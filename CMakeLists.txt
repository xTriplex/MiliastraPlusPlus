cmake_minimum_required(VERSION 3.20)

project(Miliastra++ CXX)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING
    "Supported build configurations" FORCE
)

enable_testing()
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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
)

# --------------------------------------------
# Core Static Library
# --------------------------------------------

add_library(${PROJECT_NAME} STATIC
    ${MILIASTRA_PROJECT_FILES}
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
