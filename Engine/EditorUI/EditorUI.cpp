#include "EditorUI.h"
#include "Core/Log.h"
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
    // Viewports need extra platform hooks; keep docking only for stability
    ImGui::StyleColorsDark();

#ifdef MUK_PLATFORM_WINDOWS
    ImGui_ImplWin32_Init(hwnd);

    if (rhi && rhi->GetDevice() && rhi->GetImGuiSrvHeap()) {
        // Font texture uses descriptor slot 0 on the shader-visible SRV heap
        D3D12_CPU_DESCRIPTOR_HANDLE fontCpu = rhi->GetImGuiSrvHeap()->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE fontGpu = rhi->GetImGuiSrvHeap()->GetGPUDescriptorHandleForHeapStart();

        ImGui_ImplDX12_Init(
            rhi->GetDevice(),
            (int)DX12RHI::GetFrameCount(),
            rhi->GetBackBufferFormat(),
            rhi->GetImGuiSrvHeap(),
            fontCpu,
            fontGpu
        );
        m_ImGuiDx12 = true;
        Log("EditorUI: ImGui DX12 backend + font SRV heap ready");
        MUK_CORE_INFO("ImGui DX12 font SRV heap initialized");
    } else {
        Log("EditorUI: ImGui Win32 only (no DX12 device for fonts)");
    }
#endif

    m_Initialized = true;
#else
    (void)hwnd;
    (void)rhi;
    m_Initialized = true;
    Log("EditorUI console mode - build with -DMUK_USE_IMGUI=ON");
#endif
}

void EditorUI::Shutdown() {
#ifdef MUK_USE_IMGUI
#ifdef MUK_PLATFORM_WINDOWS
    if (m_ImGuiDx12) {
        ImGui_ImplDX12_Shutdown();
        m_ImGuiDx12 = false;
    }
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
    m_WantsCaptureMouse = ImGui::GetIO().WantCaptureMouse;
    m_WantsCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
#endif
}

void EditorUI::RenderDrawData() {
#ifdef MUK_USE_IMGUI
    if (!m_Initialized || !m_ImGuiDx12 || !m_RHI) return;
    ImGui::Render();
    auto* cmd = m_RHI->GetCommandList();
    // Heap already bound in DX12RHI::BeginFrame
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
#endif
}

void EditorUI::EndFrame() {
    // Draw data already recorded in RenderDrawData
}

void EditorUI::DrawDockspace() {
#ifdef MUK_USE_IMGUI
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpaceHost", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MukDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Level");
            ImGui::MenuItem("Open Level...");
            ImGui::MenuItem("Save");
            ImGui::Separator();
            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Hierarchy");
            ImGui::MenuItem("Details");
            ImGui::MenuItem("Viewport");
            ImGui::MenuItem("Content Browser");
            ImGui::MenuItem("Console");
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
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (e.Handle.GetID() == selected.GetID())
            flags |= ImGuiTreeNodeFlags_Selected;
        ImGui::TreeNodeEx((void*)(uintptr_t)e.Handle.GetID(), flags, "%s", e.Name.c_str());
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
        ImGui::Text("Entity ID: %u", selected.GetID());
        if (auto* t = world.GetComponent<Transform>(selected)) {
            ImGui::Separator();
            ImGui::Text("Transform");
            ImGui::DragFloat3("Position", &t->Position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &t->Rotation.x, 0.5f);
            ImGui::DragFloat3("Scale", &t->Scale.x, 0.05f);
        }
        if (auto* mr = world.GetComponent<MeshRenderer>(selected)) {
            ImGui::Separator();
            ImGui::Text("Mesh: %s", mr->MeshName.c_str());
            ImGui::Text("Material: %s", mr->MaterialName.c_str());
        }
        if (auto* cam = world.GetComponent<Camera>(selected)) {
            ImGui::Separator();
            ImGui::DragFloat("FOV", &cam->FOV, 0.5f, 10.f, 120.f);
            ImGui::Checkbox("Primary", &cam->Primary);
        }
    } else {
        ImGui::TextDisabled("No entity selected");
    }
    ImGui::End();
#else
    (void)world; (void)selected;
#endif
}

void EditorUI::DrawContentBrowser() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Content Browser");
    if (ImGui::TreeNode("Meshes")) {
        ImGui::BulletText("Triangle / Cube / Quad");
        ImGui::BulletText("glTF imports (cached by path)");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Materials")) {
        ImGui::BulletText("Default / PBR from glTF");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Textures")) {
        ImGui::BulletText("Albedo / Normal from glTF");
        ImGui::TreePop();
    }
    ImGui::End();
#endif
}

void EditorUI::DrawConsole() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Console");
    for (const auto& line : m_ConsoleLines)
        ImGui::TextUnformatted(line.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::End();
#endif
}

void EditorUI::DrawViewportPlaceholder() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Viewport");
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::Text("Scene (main swapchain)  %dx%d", (int)size.x, (int)size.y);
    ImGui::TextDisabled("Depth buffer active | Camera MVP active");
    ImGui::End();
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
