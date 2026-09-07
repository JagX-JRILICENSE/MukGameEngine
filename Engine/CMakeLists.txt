set(ENGINE_SOURCES
    Core/Application.cpp
    Core/Log.cpp
    Core/Window.cpp
    Math/Vector.cpp
    Math/Matrix.cpp
    ECS/World.cpp
    ECS/Entity.cpp
    RHI/RHI.cpp
    Renderer/Renderer.cpp
    Input/Input.cpp
)

set(ENGINE_HEADERS
    Core/Application.h
    Core/Log.h
    Core/Window.h
    Core/Core.h
    Math/Vector.h
    Math/Matrix.h
    Math/Math.h
    ECS/World.h
    ECS/Entity.h
    ECS/Component.h
    RHI/RHI.h
    Renderer/Renderer.h
    Input/Input.h
    Engine.h
)

add_library(MukEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(MukEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(MukEngine PUBLIC d3d12 dxgi dxguid d3dcompiler)
endif()

# Precompiled header opportunity later
