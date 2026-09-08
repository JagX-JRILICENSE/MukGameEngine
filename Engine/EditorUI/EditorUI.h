#pragma once

#include "Core/Core.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include <string>
#include <vector>

namespace Muk {

struct EditorEntityInfo {
    Entity Handle;
    std::string Name;
    bool Selected = false;
};

class DX12RHI;

class EditorUI {
public:
    void Initialize(void* hwnd, DX12RHI* rhi);
    void Shutdown();

    void BeginFrame();
    // Call after scene draws, before RHI EndFrame - records ImGui into cmd list
    void RenderDrawData();
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
    bool m_ImGuiDx12 = false;
    bool m_WantsCaptureMouse = false;
    bool m_WantsCaptureKeyboard = false;
    DX12RHI* m_RHI = nullptr;
    std::vector<std::string> m_ConsoleLines;
    static constexpr size_t MaxConsoleLines = 200;
};

} // namespace Muk
