cmake_minimum_required(VERSION 3.20)
project(MukGameEngine VERSION 0.14.0 LANGUAGES CXX)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(MUK_BUILD_EDITOR "Build the Muk Editor" ON)
option(MUK_BUILD_SAMPLES "Build sample projects" OFF)
option(MUK_USE_DX12 "Enable DirectX 12 backend (Windows)" ON)
option(MUK_USE_VULKAN "Enable Vulkan backend" OFF)
option(MUK_USE_IMGUI "Fetch and enable Dear ImGui docking" OFF)
option(MUK_USE_JOLT "Fetch and enable Jolt Physics" OFF)
option(MUK_USE_TINYGLTF "Fetch and enable tinygltf for glTF loading" OFF)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
# Avoid interactive git prompts in CI
set(FETCHCONTENT_UPDATES_DISCONNECTED OFF)

if(WIN32)
    add_compile_definitions(MUK_PLATFORM_WINDOWS)
    if(MUK_USE_DX12)
        add_compile_definitions(MUK_RHI_DX12)
    endif()
endif()

if(MUK_USE_IMGUI)
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    FetchContent_Declare(imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(imguizmo)
    if(NOT imguizmo_POPULATED)
        FetchContent_Populate(imguizmo)
    endif()

    add_compile_definitions(MUK_USE_IMGUI)
    message(STATUS "ImGui (docking) + ImGuizmo: enabled")
endif()

if(MUK_USE_JOLT)
    FetchContent_Declare(JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.2.0
        SOURCE_SUBDIR Build
        GIT_SHALLOW TRUE
    )
    set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
    set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
    set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
    set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(JoltPhysics)
    add_compile_definitions(MUK_USE_JOLT)
endif()

if(MUK_USE_TINYGLTF)
    FetchContent_Declare(tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG v2.9.3
        GIT_SHALLOW TRUE
    )
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

if(MUK_BUILD_SAMPLES AND EXISTS "${CMAKE_SOURCE_DIR}/Samples/CMakeLists.txt")
    add_subdirectory(Samples)
endif()

message(STATUS "Muk Game Engine ${PROJECT_VERSION}")
message(STATUS "  Editor: ${MUK_BUILD_EDITOR}")
message(STATUS "  DX12: ${MUK_USE_DX12}")
message(STATUS "  ImGui: ${MUK_USE_IMGUI}")
message(STATUS "  Jolt: ${MUK_USE_JOLT}")
message(STATUS "  tinygltf: ${MUK_USE_TINYGLTF}")
