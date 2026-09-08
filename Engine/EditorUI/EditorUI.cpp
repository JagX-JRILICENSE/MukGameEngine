#include "EditorUI.h"
#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "RHI/DX12/DX12RHI.h"

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#ifdef MUK_PLATFORM_WINDOWS
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#endif
#endif

namespace Muk {

void EditorUI::Initialize(void* hwnd, DX12RHI* rhi) {
    m_RHI = rhi;
#ifdef MUK_USE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
#ifdef MUK_PLATFORM_WINDOWS
    ImGui_ImplWin32_Init(hwnd);
    if (rhi && rhi->GetDevice() && rhi->GetImGuiSrvHeap()) {
        auto fontCpu = rhi->GetImGuiSrvHeap()->GetCPUDescriptorHandleForHeapStart();
        auto fontGpu = rhi->GetImGuiSrvHeap()->GetGPUDescriptorHandleForHeapStart();
        ImGui_ImplDX12_Init(rhi->GetDevice(), (int)DX12RHI::GetFrameCount(),
                            rhi->GetBackBufferFormat(), rhi->GetImGuiSrvHeap(), fontCpu, fontGpu);
        m_ImGuiDx12 = true;
        Log("ImGui DX12 + font SRV ready");
    }
#endif
    m_Initialized = true;
#else
    (void)hwnd; (void)rhi;
    m_Initialized = true;
    Log("Console mode (enable MUK_USE_IMGUI)");
#endif
}

void EditorUI::Shutdown() {
#ifdef MUK_USE_IMGUI
#ifdef MUK_PLATFORM_WINDOWS
    if (m_ImGuiDx12) { ImGui_ImplDX12_Shutdown(); m_ImGuiDx12 = false; }
    ImGui_ImplWin32_Shutdown();
#endif
    ImGui::DestroyContext();
#endif
    m_RHI = nullptr;
    m_Initialized = false;
}

void EditorUI::BeginFrame() {
#ifdef MUK_USE_IMGUI
    if (!m_Initialized) return;
#ifdef MUK_PLATFORM_WINDOWS
    if (m_ImGuiDx12) ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
#endif
    ImGui::NewFrame();
#endif
}

void EditorUI::RenderDrawData() {
#ifdef MUK_USE_IMGUI
    if (!m_Initialized || !m_ImGuiDx12 || !m_RHI) return;
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_RHI->GetCommandList());
#endif
}

void EditorUI::EndFrame() {}

void EditorUI::DrawDockspace() {
#ifdef MUK_USE_IMGUI
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("DockSpaceHost", nullptr, flags);
    ImGui::PopStyleVar(3);
    ImGui::DockSpace(ImGui::GetID("MukDockSpace"));
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) { ImGui::MenuItem("Exit"); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Hierarchy"); ImGui::MenuItem("Details");
            ImGui::MenuItem("Viewport"); ImGui::MenuItem("Console");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();
#endif
}

void EditorUI::DrawHierarchy(std::vector<EditorEntityInfo>& entities, Entity& selected) {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Hierarchy");
    for (auto& e : entities) {
        ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (e.Handle.GetID() == selected.GetID()) f |= ImGuiTreeNodeFlags_Selected;
        ImGui::TreeNodeEx((void*)(uintptr_t)e.Handle.GetID(), f, "%s", e.Name.c_str());
        if (ImGui::IsItemClicked()) selected = e.Handle;
    }
    ImGui::End();
#else
    (void)entities; (void)selected;
#endif
}

void EditorUI::DrawDetails(World& world, Entity selected) {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Details");
    if (selected.IsValid()) {
        ImGui::Text("Entity %u", selected.GetID());
        if (auto* t = world.GetComponent<Transform>(selected)) {
            ImGui::DragFloat3("Position", &t->Position.x, 0.1f);
            ImGui::DragFloat3("Scale", &t->Scale.x, 0.05f);
        }
    } else ImGui::TextDisabled("No selection");
    ImGui::End();
#else
    (void)world; (void)selected;
#endif
}

void EditorUI::DrawContentBrowser() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Content Browser");
    ImGui::BulletText("Meshes / Materials / Textures (glTF)");
    ImGui::End();
#endif
}

void EditorUI::DrawConsole() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Console");
    for (const auto& line : m_ConsoleLines)
        ImGui::TextUnformatted(line.c_str());
    ImGui::End();
#endif
}

void EditorUI::DrawViewport(Renderer& renderer, void* sceneRtGpuHandle, u32 rtW, u32 rtH) {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Viewport");
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x < 1) size.x = 1;
    if (size.y < 1) size.y = 1;
    m_ViewportW = (u32)size.x;
    m_ViewportH = (u32)size.y;

    if (sceneRtGpuHandle && rtW > 0 && rtH > 0) {
        ImGui::Image(sceneRtGpuHandle, size);
    } else {
        ImGui::TextDisabled("SceneRT not ready (%dx%d desired)", m_ViewportW, m_ViewportH);
        ImGui::Text("Call EnsureSceneRT + BeginSceneRT before DrawViewport");
    }
    (void)renderer;
    ImGui::End();
#else
    (void)renderer; (void)sceneRtGpuHandle; (void)rtW; (void)rtH;
#endif
}

void EditorUI::Log(const std::string& message) {
    m_ConsoleLines.push_back(message);
    if (m_ConsoleLines.size() > MaxConsoleLines)
        m_ConsoleLines.erase(m_ConsoleLines.begin());
#ifndef MUK_USE_IMGUI
    MUK_CORE_INFO("[Editor] {0}", message.c_str());
#endif
}

} // namespace Muk
