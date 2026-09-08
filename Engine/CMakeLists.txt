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
    RHI/DX12/DX12Pipeline.cpp
    Renderer/Renderer.cpp
    Renderer/Mesh.cpp
    Input/Input.cpp
    Physics/PhysicsWorld.cpp
    Asset/AssetManager.cpp
    Asset/GltfLoader.cpp
    EditorUI/EditorUI.cpp
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
    RHI/DX12/DX12Pipeline.h
    Renderer/Renderer.h
    Renderer/Mesh.h
    Renderer/Material.h
    Input/Input.h
    Physics/PhysicsWorld.h
    Asset/AssetManager.h
    Asset/GltfLoader.h
    EditorUI/EditorUI.h
    Engine.h
)

add_library(MukEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(MukEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(MukEngine PUBLIC d3d12 dxgi dxguid d3dcompiler)
endif()

if(MUK_USE_IMGUI)
    target_include_directories(MukEngine PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_sources(MukEngine PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    )
endif()

if(MUK_USE_JOLT)
    target_link_libraries(MukEngine PUBLIC Jolt)
endif()

if(MUK_USE_TINYGLTF)
    target_include_directories(MukEngine PUBLIC ${tinygltf_SOURCE_DIR})
endif()
