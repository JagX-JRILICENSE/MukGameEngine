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
class Renderer;

class EditorUI {
public:
    void Initialize(void* hwnd, DX12RHI* rhi);
    void Shutdown();

    void BeginFrame();
    void RenderDrawData();
    void EndFrame();

    void DrawDockspace();
    void DrawHierarchy(std::vector<EditorEntityInfo>& entities, Entity& selected);
    void DrawDetails(World& world, Entity selected);
    void DrawContentBrowser();
    void DrawConsole();

    // Returns desired viewport size; call EnsureSceneRT + render scene before Image
    void DrawViewport(Renderer& renderer, void* sceneRtGpuHandle, u32 rtW, u32 rtH);

    u32 GetDesiredViewportWidth() const { return m_ViewportW; }
    u32 GetDesiredViewportHeight() const { return m_ViewportH; }

    bool IsInitialized() const { return m_Initialized; }
    void Log(const std::string& message);

private:
    bool m_Initialized = false;
    bool m_ImGuiDx12 = false;
    DX12RHI* m_RHI = nullptr;
    std::vector<std::string> m_ConsoleLines;
    u32 m_ViewportW = 800;
    u32 m_ViewportH = 450;
    static constexpr size_t MaxConsoleLines = 200;
};

} // namespace Muk
