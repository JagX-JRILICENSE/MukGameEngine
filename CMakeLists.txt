cmake_minimum_required(VERSION 3.25)
project(MukGameEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(MUK_BUILD_EDITOR "Build the Muk Editor" ON)
option(MUK_BUILD_SAMPLES "Build sample projects" ON)
option(MUK_USE_DX12 "Enable DirectX 12 backend (Windows)" ON)
option(MUK_USE_VULKAN "Enable Vulkan backend" OFF)

# Platform detection
if(WIN32)
    add_compile_definitions(MUK_PLATFORM_WINDOWS)
    if(MUK_USE_DX12)
        add_compile_definitions(MUK_RHI_DX12)
    endif()
endif()

# Global include
include_directories(${CMAKE_SOURCE_DIR}/Engine)

# Engine library
add_subdirectory(Engine)

# Runtime
add_subdirectory(Runtime)

# Editor
if(MUK_BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

# Samples
if(MUK_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()

message(STATUS "Muk Game Engine ${PROJECT_VERSION}")
message(STATUS "  Editor: ${MUK_BUILD_EDITOR}")
message(STATUS "  Samples: ${MUK_BUILD_SAMPLES}")
message(STATUS "  DX12: ${MUK_USE_DX12}")
