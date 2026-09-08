#include "EditorUI.h"
#include "Core/Log.h"

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#ifdef MUK_PLATFORM_WINDOWS
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#endif
#endif

namespace Muk {

void EditorUI::Initialize(void* hwnd, void* d3d12Device, void* d3d12CommandQueue) {
#ifdef MUK_USE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();

#ifdef MUK_PLATFORM_WINDOWS
    ImGui_ImplWin32_Init(hwnd);
    // DX12 backend needs descriptor heap setup - simplified init marker
    // Full descriptor heap wiring is done when MUK_USE_IMGUI + DX12 are both on.
    (void)d3d12Device;
    (void)d3d12CommandQueue;
    // ImGui_ImplDX12_Init(...) requires SRV heap - see Docs/ImGuiSetup.md
#endif

    m_Initialized = true;
    Log("EditorUI: Dear ImGui initialized with docking");
    MUK_CORE_INFO("EditorUI: ImGui docking enabled");
#else
    (void)hwnd;
    (void)d3d12Device;
    (void)d3d12CommandQueue;
    m_Initialized = true;
    Log("EditorUI: Running without ImGui (console mode). Enable MUK_USE_IMGUI + FetchContent.");
    MUK_CORE_INFO("EditorUI: console fallback mode (build with -DMUK_USE_IMGUI=ON)");
#endif
}

void EditorUI::Shutdown() {
#ifdef MUK_USE_IMGUI
#ifdef MUK_PLATFORM_WINDOWS
    // ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
#endif
    ImGui::DestroyContext();
#endif
    m_Initialized = false;
}

void EditorUI::BeginFrame() {
#ifdef MUK_USE_IMGUI
    if (!m_Initialized) return;
#ifdef MUK_PLATFORM_WINDOWS
    ImGui_ImplWin32_NewFrame();
    // ImGui_ImplDX12_NewFrame();
#endif
    ImGui::NewFrame();
    m_WantsCaptureMouse = ImGui::GetIO().WantCaptureMouse;
    m_WantsCaptureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
#endif
}

void EditorUI::EndFrame() {
#ifdef MUK_USE_IMGUI
    if (!m_Initialized) return;
    ImGui::Render();
    // ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
#endif
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
            if (ImGui::MenuItem("New Level")) {}
            if (ImGui::MenuItem("Open Level...")) {}
            if (ImGui::MenuItem("Save")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Hierarchy", nullptr, true);
            ImGui::MenuItem("Details", nullptr, true);
            ImGui::MenuItem("Viewport", nullptr, true);
            ImGui::MenuItem("Content Browser", nullptr, true);
            ImGui::MenuItem("Console", nullptr, true);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();
#else
    // Console mode: nothing to draw
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
        if (ImGui::IsItemClicked()) {
            selected = e.Handle;
            e.Selected = true;
        }
    }
    ImGui::End();
#else
    (void)entities;
    (void)selected;
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
            ImGui::Text("MeshRenderer");
            ImGui::Text("Mesh: %s", mr->MeshName.c_str());
            ImGui::Text("Material: %s", mr->MaterialName.c_str());
        }
        if (auto* cam = world.GetComponent<Camera>(selected)) {
            ImGui::Separator();
            ImGui::Text("Camera");
            ImGui::DragFloat("FOV", &cam->FOV, 0.5f, 10.0f, 120.0f);
            ImGui::DragFloat("Near", &cam->Near, 0.01f);
            ImGui::DragFloat("Far", &cam->Far, 1.0f);
            ImGui::Checkbox("Primary", &cam->Primary);
        }
    } else {
        ImGui::TextDisabled("No entity selected");
    }
    ImGui::End();
#else
    (void)world;
    (void)selected;
#endif
}

void EditorUI::DrawContentBrowser() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Content Browser");
    ImGui::Text("Assets/");
    if (ImGui::TreeNode("Meshes")) {
        ImGui::BulletText("Triangle");
        ImGui::BulletText("Cube");
        ImGui::BulletText("Quad");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Materials")) {
        ImGui::BulletText("Default");
        ImGui::BulletText("Red / Green / Blue");
        ImGui::TreePop();
    }
    ImGui::End();
#endif
}

void EditorUI::DrawConsole() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Console");
    for (const auto& line : m_ConsoleLines) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::End();
#endif
}

void EditorUI::DrawViewportPlaceholder() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Viewport");
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::Text("3D Viewport (%dx%d)", (int)size.x, (int)size.y);
    ImGui::TextDisabled("Scene renders to the main swapchain for now.");
    ImGui::TextDisabled("Future: render-to-texture displayed here.");
    ImGui::End();
#endif
}

void EditorUI::Log(const std::string& message) {
    m_ConsoleLines.push_back(message);
    if (m_ConsoleLines.size() > MaxConsoleLines)
        m_ConsoleLines.erase(m_ConsoleLines.begin());
#ifdef MUK_USE_IMGUI
    // already stored for panel
#else
    MUK_CORE_INFO("[Editor] {0}", message.c_str());
#endif
}

} // namespace Muk
