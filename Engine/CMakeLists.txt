set(ENGINE_SOURCES
    Core/Application.cpp
    Core/Log.cpp
    Core/Window.cpp
    Math/Vector.cpp
    Math/Matrix.cpp
    ECS/World.cpp
    ECS/Entity.cpp
    RHI/RHI.cpp
    RHI/DX12/DX12RHI.cpp
    Renderer/Renderer.cpp
    Renderer/Mesh.cpp
    Input/Input.cpp
    Physics/PhysicsWorld.cpp
    Asset/AssetManager.cpp
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
    RHI/DX12/DX12RHI.h
    Renderer/Renderer.h
    Renderer/Mesh.h
    Renderer/Material.h
    Input/Input.h
    Physics/PhysicsWorld.h
    Asset/AssetManager.h
    Engine.h
)

add_library(MukEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(MukEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(MukEngine PUBLIC d3d12 dxgi dxguid d3dcompiler)
endif()
