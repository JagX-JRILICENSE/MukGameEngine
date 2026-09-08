#pragma once

#include "Core/Core.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include <string>
#include <vector>
#include <functional>

namespace Muk {

struct EditorEntityInfo {
    Entity Handle;
    std::string Name;
    bool Selected = false;
};

/**
 * Editor UI layer.
 * When MUK_USE_IMGUI is defined and Dear ImGui is linked, draws real docking panels.
 * Otherwise falls back to console logging of panel state.
 *
 * Panels:
 *  - Viewport
 *  - Hierarchy (entity list)
 *  - Details (inspector for selected entity)
 *  - Content Browser
 *  - Console
 */
class EditorUI {
public:
    void Initialize(void* hwnd, void* d3d12Device, void* d3d12CommandQueue);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void DrawDockspace();
    void DrawHierarchy(std::vector<EditorEntityInfo>& entities, Entity& selected);
    void DrawDetails(World& world, Entity selected);
    void DrawContentBrowser();
    void DrawConsole();
    void DrawViewportPlaceholder();

    bool IsInitialized() const { return m_Initialized; }
    bool WantsCaptureMouse() const { return m_WantsCaptureMouse; }
    bool WantsCaptureKeyboard() const { return m_WantsCaptureKeyboard; }

    void Log(const std::string& message);

private:
    bool m_Initialized = false;
    bool m_WantsCaptureMouse = false;
    bool m_WantsCaptureKeyboard = false;
    std::vector<std::string> m_ConsoleLines;
    static constexpr size_t MaxConsoleLines = 200;
};

} // namespace Muk
