cmake_minimum_required(VERSION 3.20)
project(MukGameEngine VERSION 0.15.0 LANGUAGES CXX)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)

option(MUK_BUILD_EDITOR "Build Muk Editor application" ON)
option(MUK_BUILD_SAMPLES "Build samples" OFF)
option(MUK_USE_DX12 "DirectX 12" ON)
option(MUK_USE_IMGUI "Dear ImGui" OFF)
option(MUK_USE_JOLT "Jolt Physics" OFF)
option(MUK_USE_TINYGLTF "tinygltf" OFF)
# STUB disabled — we ship a real windowed DX12 app
option(MUK_CI_STUB "DEPRECATED stub" OFF)

if(MUK_CI_STUB)
    message(WARNING "MUK_CI_STUB is deprecated; building real editor instead")
endif()

include(FetchContent)

if(WIN32)
    add_compile_definitions(MUK_PLATFORM_WINDOWS)
    if(MUK_USE_DX12)
        add_compile_definitions(MUK_RHI_DX12)
    endif()
endif()

if(MUK_USE_IMGUI)
    FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking GIT_SHALLOW TRUE)
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()
    FetchContent_Declare(imguizmo GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git GIT_TAG master GIT_SHALLOW TRUE)
    FetchContent_GetProperties(imguizmo)
    if(NOT imguizmo_POPULATED)
        FetchContent_Populate(imguizmo)
    endif()
    add_compile_definitions(MUK_USE_IMGUI)
endif()

if(MUK_USE_TINYGLTF)
    FetchContent_Declare(tinygltf GIT_REPOSITORY https://github.com/syoyo/tinygltf.git GIT_TAG v2.9.3 GIT_SHALLOW TRUE)
    FetchContent_GetProperties(tinygltf)
    if(NOT tinygltf_POPULATED)
        FetchContent_Populate(tinygltf)
    endif()
    add_compile_definitions(MUK_USE_TINYGLTF)
endif()

include_directories(${CMAKE_SOURCE_DIR}/Engine)

add_subdirectory(Engine)
add_subdirectory(Runtime)

if(MUK_BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

message(STATUS "Muk Game Engine ${PROJECT_VERSION} — REAL application build")
