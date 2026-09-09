# Minimal reliable sources for Windows CI (expand later)
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
    EditorUI/EditorUI.cpp
    EditorUI/ViewportGizmo.cpp
    Editor/UndoStack.cpp
    Editor/PlayInEditor.cpp
    Editor/Screenshot.cpp
    AI/UserSettings.cpp
    AI/AIClient.cpp
    AI/AIControlPanel.cpp
    Scene/Prefab.cpp
    Scene/SceneSerializer.cpp
    Audio/AudioSystem.cpp
    Particles/ParticleSystem.cpp
    Gameplay/Collectible.cpp
)

add_library(MukEngine STATIC ${ENGINE_SOURCES})
target_include_directories(MukEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(MukEngine PUBLIC d3d12 dxgi dxguid d3dcompiler winhttp shell32 ole32)
endif()

if(MUK_USE_IMGUI)
    target_include_directories(MukEngine PUBLIC
        ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${imguizmo_SOURCE_DIR})
    target_sources(MukEngine PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)
endif()

if(MUK_USE_JOLT)
    target_link_libraries(MukEngine PUBLIC Jolt)
endif()
if(MUK_USE_TINYGLTF)
    target_include_directories(MukEngine PUBLIC ${tinygltf_SOURCE_DIR})
endif()
