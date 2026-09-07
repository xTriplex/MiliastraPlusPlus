cmake_minimum_required(VERSION 3.20)

project(Miliastra++ CXX)

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

file(GLOB_RECURSE MILIASTRA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Miliastra++/Source/*.cpp
)

# --------------------------------------------
# Main Executable
# --------------------------------------------

add_executable(${PROJECT_NAME}
    ${MILIASTRA_SOURCES}
)

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
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)
