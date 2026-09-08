set(ENGINE_SOURCES
    Core/Application.cpp
    Core/Log.cpp
    Core/Window.cpp
    Core/Profiler.cpp
    Math/Vector.cpp
    Math/Matrix.cpp
    ECS/World.cpp
    ECS/Entity.cpp
    RHI/RHI.cpp
    RHI/DX12/DX12RHI.cpp
    RHI/DX12/DX12Pipeline.cpp
    RHI/DX12/DX12Texture.cpp
    RHI/DX12/DX12SceneRT.cpp
    RHI/DX12/DX12ShadowMap.cpp
    Renderer/Renderer.cpp
    Renderer/Mesh.cpp
    Input/Input.cpp
    Physics/PhysicsWorld.cpp
    Physics/CharacterController.cpp
    Asset/AssetManager.cpp
    Asset/GltfLoader.cpp
    Asset/ContentBrowser.cpp
    EditorUI/EditorUI.cpp
    EditorUI/ViewportGizmo.cpp
    Editor/UndoStack.cpp
    Editor/PlayInEditor.cpp
    AI/UserSettings.cpp
    AI/AIClient.cpp
    AI/AIControlPanel.cpp
    AI/AIGameAgent.cpp
    Scene/SceneRenderer.cpp
    Scene/SceneSerializer.cpp
    Animation/Skeleton.cpp
    Audio/AudioSystem.cpp
    Reflection/Reflection.cpp
)

set(ENGINE_HEADERS
    Core/Application.h
    Core/Log.h
    Core/Window.h
    Core/Core.h
    Core/Profiler.h
    Math/Vector.h
    Math/Matrix.h
    Math/Math.h
    ECS/World.h
    ECS/Entity.h
    ECS/Component.h
    RHI/RHI.h
    RHI/DX12/DX12RHI.h
    RHI/DX12/DX12Pipeline.h
    RHI/DX12/DX12Texture.h
    RHI/DX12/DX12SceneRT.h
    RHI/DX12/DX12ShadowMap.h
    Renderer/Renderer.h
    Renderer/Mesh.h
    Renderer/Material.h
    Renderer/Texture.h
    Renderer/PostSettings.h
    Input/Input.h
    Physics/PhysicsWorld.h
    Physics/CharacterController.h
    Asset/AssetManager.h
    Asset/GltfLoader.h
    Asset/ContentBrowser.h
    EditorUI/EditorUI.h
    EditorUI/ViewportGizmo.h
    Editor/UndoStack.h
    Editor/PlayInEditor.h
    AI/UserSettings.h
    AI/AIClient.h
    AI/AIControlPanel.h
    AI/AIGameAgent.h
    Scene/SceneRenderer.h
    Scene/SceneSerializer.h
    Animation/Skeleton.h
    Audio/AudioSystem.h
    Reflection/Reflection.h
    Engine.h
)

add_library(MukEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(MukEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(MukEngine PUBLIC d3d12 dxgi dxguid d3dcompiler winhttp shell32 ole32 xaudio2)
endif()

if(MUK_USE_IMGUI)
    target_include_directories(MukEngine PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imguizmo_SOURCE_DIR}
    )
    target_sources(MukEngine PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    )
endif()

if(MUK_USE_JOLT)
    target_link_libraries(MukEngine PUBLIC Jolt)
endif()

if(MUK_USE_TINYGLTF)
    target_include_directories(MukEngine PUBLIC ${tinygltf_SOURCE_DIR})
endif()
