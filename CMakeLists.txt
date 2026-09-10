cmake_minimum_required(VERSION 3.20)

project(Miliastra++ CXX)

# Keep the Visual Studio solution deliberately limited to the configurations
# supported by the project.  Dist must be a real multi-configuration entry,
# rather than only appearing in generator expressions.
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING
    "Supported build configurations" FORCE
)

# Make the application the default Visual Studio startup project.  This is
# initialized before the Visual Studio solution is generated so utility
# targets such as ALL_BUILD are never selected for F5 by default.
set(CMAKE_VS_STARTUP_PROJECT "${PROJECT_NAME}")

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
# Main Executable
# --------------------------------------------

add_executable(${PROJECT_NAME}
    ${MILIASTRA_PROJECT_FILES}
)

add_executable(Phase1Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Phase1Tests.cpp
)

target_include_directories(Phase1Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor
)

add_test(NAME Phase1Tests COMMAND Phase1Tests)

add_executable(Phase2Milestone1Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Phase2Milestone1Tests.cpp
)

target_include_directories(Phase2Milestone1Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
)

add_test(NAME Phase2Milestone1Tests COMMAND Phase2Milestone1Tests)

add_executable(Phase2Milestone2Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Phase2Milestone2Tests.cpp
)

target_include_directories(Phase2Milestone2Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
)

add_test(NAME Phase2Milestone2Tests COMMAND Phase2Milestone2Tests)

add_executable(Phase2Milestone3Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Phase2Milestone3Tests.cpp
)

target_include_directories(Phase2Milestone3Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
)

add_test(NAME Phase2Milestone3Tests COMMAND Phase2Milestone3Tests)

add_executable(Phase2Milestone4Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Phase2Milestone4Tests.cpp
)

target_include_directories(Phase2Milestone4Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
)

add_test(NAME Phase2Milestone4Tests COMMAND Phase2Milestone4Tests)

add_executable(Phase2Milestone5Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Phase2Milestone5Tests.cpp
)

target_include_directories(Phase2Milestone5Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
)

add_test(NAME Phase2Milestone5Tests COMMAND Phase2Milestone5Tests)

# --------------------------------------------
# Include Directories
# --------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/Private
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Vendor
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

    # Enable multi-processor compilation.
    # CMake's MSVC generator handles this through /MP.
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

# --------------------------------------------
# Link Libraries
# --------------------------------------------

target_link_libraries(${PROJECT_NAME} PRIVATE
    $<$<PLATFORM_ID:Windows>:opengl32.lib>
)

# --------------------------------------------
# Visual Studio Linker Settings
# --------------------------------------------

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "/ENTRY:mainCRTStartup"
)

set_property(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}"
)
