#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "EditorUI/ViewportGizmo.h"
#include "AI/AIControlPanel.h"
#include "Physics/CharacterController.h"
#include "Core/Profiler.h"
#include "RHI/DX12/DX12RHI.h"
#include "Input/Input.h"

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

using namespace Muk;

class EditorApp : public Application {
protected:
    void OnInit() override {
        auto* dx = dynamic_cast<DX12RHI*>(Renderer().GetRHI());
        m_UI.Initialize(Window().GetNativeHandle(), dx);
        m_AI.Initialize();

        CameraView cam;
        cam.Eye = {0.0f, 3.0f, -8.0f};
        cam.Target = {0.0f, 0.5f, 0.0f};
        Renderer().SetCamera(cam);

        // Sun
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Sun";
            auto& L = ECS().AddComponent<DirectionalLight>(e);
            L.Direction = {0.45f, -1.0f, 0.35f};
            L.Intensity = 1.5f;
            L.Ambient = 0.18f;
            Track(e, "Sun");
        }

        // Floor (static physics)
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Floor";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {0, -0.1f, 0};
            t.Scale = {12, 0.2f, 12};
            auto& mr = ECS().AddComponent<MeshRenderer>(e);
            mr.MeshName = "Cube";
            mr.MaterialName = "Default";
            Track(e, "Floor");

            RigidBodyDesc rb;
            rb.Type = BodyType::Static;
            rb.Shape = ShapeType::Box;
            rb.Position = t.Position;
            rb.HalfExtents = {6, 0.1f, 6};
            auto bodyId = Physics().CreateBody(rb);
            ECS().AddComponent<RigidBodyComponent>(e).BodyId = bodyId;
        }

        // Cubes
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Cube";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {1.5f, 0.5f, 0};
            auto& mr = ECS().AddComponent<MeshRenderer>(e);
            mr.MeshName = "Cube";
            mr.MaterialName = "Default";
            Track(e, "Cube");
            m_Selected = e;
        }

        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Triangle";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {-2, 0.5f, 0};
            auto& mr = ECS().AddComponent<MeshRenderer>(e);
            mr.MeshName = "Triangle";
            mr.MaterialName = "CheckerMat";
            Track(e, "Triangle");
        }

        // Character
        {
            CharacterDesc cd;
            cd.Position = {0, 1.0f, 2};
            cd.Radius = 0.3f;
            cd.Height = 1.0f;
            m_Character.Create(Physics(), cd);

            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Player";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = cd.Position;
            t.Scale = {0.5f, 1.0f, 0.5f};
            auto& mr = ECS().AddComponent<MeshRenderer>(e);
            mr.MeshName = "Cube";
            mr.MaterialName = "Default";
            Track(e, "Player");
            m_PlayerEntity = e;
        }

        if (auto tri = Assets().GetMesh("Triangle"))
            Renderer().UploadMesh("Triangle", *tri);
        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);

        auto checker = Texture::CreateSolid(64, 64, 200, 180, 60);
        for (u32 y = 0; y < 64; ++y)
            for (u32 x = 0; x < 64; ++x) {
                bool c = ((x / 8) + (y / 8)) & 1;
                u32 i = (y * 64 + x) * 4;
                checker->Pixels[i+0] = c ? 220 : 40;
                checker->Pixels[i+1] = c ? 180 : 40;
                checker->Pixels[i+2] = c ? 40 : 120;
                checker->Pixels[i+3] = 255;
            }
        checker->Name = "Checker";
        Renderer().UploadTexture("Checker", *checker);
        m_CheckerMat = Material::CreateDefault();
        m_CheckerMat.Name = "CheckerMat";
        m_CheckerMat.AlbedoMap = checker;
        m_CheckerMat.AlbedoTexture = "Checker";

        m_UI.Log(Physics().IsUsingJolt()
            ? "Jolt CharacterVirtual + shadows + ImGuizmo ready"
            : "Simple character + shadows + ImGuizmo ready (enable MUK_USE_JOLT for Jolt)");
    }

    void OnUpdate(float dt) override {
        // WASD + Space for character
        Vec3 wish{0, 0, 0};
#ifdef MUK_PLATFORM_WINDOWS
        if (GetAsyncKeyState('W') & 0x8000) wish.z += 1;
        if (GetAsyncKeyState('S') & 0x8000) wish.z -= 1;
        if (GetAsyncKeyState('A') & 0x8000) wish.x -= 1;
        if (GetAsyncKeyState('D') & 0x8000) wish.x += 1;
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) m_Character.Jump(6.0f);
#endif
        m_Character.SetMoveInput(wish, 5.0f);
        m_Character.Update(Physics(), dt);

        if (m_PlayerEntity.IsValid()) {
            if (auto* t = ECS().GetComponent<Transform>(m_PlayerEntity))
                t->Position = m_Character.GetPosition();
        }
    }

    void OnRender() override {
        m_Gizmo.BeginFrame();
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        DrawDetails();
        m_UI.DrawContentBrowser();
        m_UI.DrawConsole();
        m_AI.Draw(Renderer());
        DrawStats();

        auto drawScene = [&]() {
            Renderer().SetDirectionalLight({0.45f, -1.0f, 0.35f}, {1, 0.98f, 0.92f}, 1.5f, 0.18f);
            ECS().ForEach<Transform, MeshRenderer>([&](Entity, Transform& t, MeshRenderer& mr) {
                if (!mr.Visible) return;
                Material mat = Material::CreateDefault();
                if (mr.MaterialName == "CheckerMat") mat = m_CheckerMat;
                else if (auto m = Assets().GetMaterial(mr.MaterialName)) mat = *m;
                Renderer().DrawMesh(mr.MeshName, t.GetMatrix(), mat);
            });
        };

        // Shadow pass
        Renderer().BeginShadowPass({0, 0, 0}, 18.0f);
        drawScene();
        Renderer().EndShadowPass();

        // Scene RT (viewport)
        u32 vw = m_UI.GetDesiredViewportWidth();
        u32 vh = m_UI.GetDesiredViewportHeight();
        Renderer().EnsureSceneRT(vw, vh);
        Renderer().BeginSceneRT();
        drawScene();
        Renderer().EndSceneRT();

        m_UI.DrawViewport(Renderer(), Renderer().GetSceneRTGpuHandle(),
                          Renderer().GetSceneRTWidth(), Renderer().GetSceneRTHeight());

        // Gizmo over viewport
        DrawViewportGizmo();

        m_UI.RenderDrawData();
        m_UI.EndFrame();
    }

    void OnShutdown() override {
        m_Character.Destroy(Physics());
        m_UI.Shutdown();
    }

private:
    void Track(Entity e, const std::string& name) {
        EditorEntityInfo info;
        info.Handle = e;
        info.Name = name;
        m_Entities.push_back(info);
        m_UI.Log("Created: " + name);
    }

    void DrawDetails() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Details");
        if (m_Selected.IsValid()) {
            ImGui::Text("Entity %u", m_Selected.GetID());
            if (auto* t = ECS().GetComponent<Transform>(m_Selected)) {
                ImGui::DragFloat3("Position", &t->Position.x, 0.05f);
                ImGui::DragFloat3("Rotation", &t->Rotation.x, 0.5f);
                ImGui::DragFloat3("Scale", &t->Scale.x, 0.05f);
            }
            ImGui::Separator();
            ImGui::Text("Gizmo: T translate | R rotate | Y scale");
            if (ImGui::RadioButton("Translate", m_Gizmo.GetOperation() == GizmoOp::Translate))
                m_Gizmo.SetOperation(GizmoOp::Translate);
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", m_Gizmo.GetOperation() == GizmoOp::Rotate))
                m_Gizmo.SetOperation(GizmoOp::Rotate);
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", m_Gizmo.GetOperation() == GizmoOp::Scale))
                m_Gizmo.SetOperation(GizmoOp::Scale);
        }
        ImGui::Separator();
        ImGui::Text("Player grounded: %s", m_Character.IsGrounded() ? "yes" : "no");
        ImGui::Text("Physics: %s", Physics().IsUsingJolt() ? "Jolt" : "Simple");
        ImGui::Text("Shadows: %s", Renderer().ShadowsEnabled() ? "on" : "off");
        ImGui::End();
#else
        m_UI.DrawDetails(ECS(), m_Selected);
#endif
    }

    void DrawViewportGizmo() {
#ifdef MUK_USE_IMGUI
        if (!m_Selected.IsValid()) return;
        auto* t = ECS().GetComponent<Transform>(m_Selected);
        if (!t) return;

        ImGui::Begin("Viewport");
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 min = ImGui::GetWindowContentRegionMin();
        ImVec2 size = ImGui::GetContentRegionAvail();
        float x = pos.x + min.x;
        float y = pos.y + min.y;

        if (ImGui::IsKeyPressed(ImGuiKey_T)) m_Gizmo.SetOperation(GizmoOp::Translate);
        if (ImGui::IsKeyPressed(ImGuiKey_R)) m_Gizmo.SetOperation(GizmoOp::Rotate);
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) m_Gizmo.SetOperation(GizmoOp::Scale);

        Mat4 view = Renderer().GetViewMatrix();
        Mat4 proj = Renderer().GetProjectionMatrix();
        m_Gizmo.Draw(view.m, proj.m, x, y, size.x, size.y, *t);
        ImGui::End();
#endif
    }

    void DrawStats() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Stats");
        auto& p = Profiler::Get();
        ImGui::Text("FPS: %.1f", p.Fps());
        ImGui::Text("Frame: %.2f ms", p.LastFrameMs());
        for (auto& [name, ms] : p.LastScopes())
            ImGui::Text("%s: %.2f ms", name.c_str(), ms);
        ImGui::End();
#endif
    }

    EditorUI m_UI;
    AIControlPanel m_AI;
    ViewportGizmo m_Gizmo;
    CharacterController m_Character;
    std::vector<EditorEntityInfo> m_Entities;
    Entity m_Selected;
    Entity m_PlayerEntity;
    Material m_CheckerMat;
};

int main() {
    EditorApp app;
    app.Run();
    return 0;
}
