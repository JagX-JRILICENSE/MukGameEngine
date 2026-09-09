cmake_minimum_required(VERSION 3.20)
project(MukGameEngine VERSION 0.15.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)

option(MUK_BUILD_EDITOR "Build Muk Editor (real DX12 window)" ON)
option(MUK_BUILD_ENGINE "Build full engine library + runtime" OFF)

if(MUK_BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

# Optional full engine (local development)
if(MUK_BUILD_ENGINE)
    if(WIN32)
        add_compile_definitions(MUK_PLATFORM_WINDOWS MUK_RHI_DX12)
    endif()
    include_directories(${CMAKE_SOURCE_DIR}/Engine)
    add_subdirectory(Engine)
    add_subdirectory(Runtime)
endif()

message(STATUS "Muk ${PROJECT_VERSION} — real Windows DX12 application")
