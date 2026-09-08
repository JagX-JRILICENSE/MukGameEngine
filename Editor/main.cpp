#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "EditorUI/ViewportGizmo.h"
#include "Editor/UndoStack.h"
#include "Editor/PlayInEditor.h"
#include "AI/AIControlPanel.h"
#include "AI/AIGameAgent.h"
#include "Asset/ContentBrowser.h"
#include "Scene/SceneSerializer.h"
#include "Reflection/Reflection.h"
#include "Audio/AudioSystem.h"
#include "Animation/Skeleton.h"
#include "Physics/CharacterController.h"
#include "Core/Profiler.h"
#include "RHI/DX12/DX12RHI.h"

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>
#endif

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
        m_Agent.SetClient(const_cast<AIClient*>(&m_AIClientProxy()));
        // Use settings from AI panel path: re-bind after panel init
        m_AgentClient.SetSettings(UserSettings{});
        {
            UserSettings s;
            s.Load();
            m_AgentClient.SetSettings(s);
            m_Agent.SetClient(&m_AgentClient);
        }

        m_Audio.Initialize();
        m_Anim.EnsureDemoAssets();
        m_Content.SetRoot("Assets");
        m_Content.Rescan();

        CameraView cam;
        cam.Eye = {0.0f, 3.0f, -8.0f};
        cam.Target = {0.0f, 0.5f, 0.0f};
        Renderer().SetCamera(cam);

        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Sun";
            auto& L = ECS().AddComponent<DirectionalLight>(e);
            L.Direction = {0.45f, -1.0f, 0.35f};
            L.Intensity = 1.5f;
            L.Ambient = 0.18f;
            Track(e, "Sun");
        }

        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Floor";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {0, -0.1f, 0};
            t.Scale = {12, 0.2f, 12};
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
            Track(e, "Floor");
            RigidBodyDesc rb;
            rb.Type = BodyType::Static;
            rb.Shape = ShapeType::Box;
            rb.Position = t.Position;
            rb.HalfExtents = {6, 0.1f, 6};
            ECS().AddComponent<RigidBodyComponent>(e).BodyId = Physics().CreateBody(rb);
        }

        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Cube";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {1.5f, 0.5f, 0};
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
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

        {
            CharacterDesc cd;
            cd.Position = {0, 1.0f, 2};
            m_Character.Create(Physics(), cd);
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Player";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = cd.Position;
            t.Scale = {0.5f, 1.0f, 0.5f};
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
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
        m_CheckerMat.AlbedoMap = checker;
        m_CheckerMat.AlbedoTexture = "Checker";

        m_UI.Log("v0.7: cascades, AI builder, content browser, reflection, audio, scenes");
    }

    // helper to satisfy early SetClient - unused
    const AIClient& m_AIClientProxy() { return m_AgentClient; }

    void OnUpdate(float dt) override {
#ifdef MUK_PLATFORM_WINDOWS
        static bool zWas = false, yWas = false, pWas = false, sWas = false;
        bool zDown = (GetAsyncKeyState('Z') & 0x8000) != 0;
        bool yDown = (GetAsyncKeyState('Y') & 0x8000) != 0;
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool pDown = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        bool sDown = (GetAsyncKeyState('S') & 0x8000) != 0;

        if (ctrl && zDown && !zWas && !m_PIE.IsPlaying()) m_Undo.Undo(ECS());
        if (ctrl && yDown && !yWas && !m_PIE.IsPlaying()) m_Undo.Redo(ECS());
        if (pDown && !pWas) m_PIE.Toggle(ECS(), &m_Character);
        if (ctrl && sDown && !sWas)
            SceneSerializer::SaveWorld(ECS(), "Assets/Scenes/autosave.json", &m_Entities);
        zWas = zDown; yWas = yDown; pWas = pDown; sWas = sDown;
#endif

        // Sync AI agent keys from disk occasionally
        static float keyTimer = 0;
        keyTimer += dt;
        if (keyTimer > 2.0f) {
            keyTimer = 0;
            UserSettings s; s.Load();
            m_AgentClient.SetSettings(s);
        }

        m_Agent.Tick(ECS(), Renderer(), &m_Audio, m_Entities, m_Selected);

        m_Audio.SetListener({ Renderer().GetCamera().Eye, {0,0,1}, {0,1,0} });
        m_Audio.Update(dt);

        Vec3 wish{0, 0, 0};
#ifdef MUK_PLATFORM_WINDOWS
        if (GetAsyncKeyState('W') & 0x8000) wish.z += 1;
        if (GetAsyncKeyState('S') & 0x8000) wish.z -= 1;
        if (GetAsyncKeyState('A') & 0x8000) wish.x -= 1;
        if (GetAsyncKeyState('D') & 0x8000) wish.x += 1;
        if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && m_PIE.IsPlaying())
            m_Character.Jump(6.0f);
#endif
        m_Character.SetMoveInput(wish, 5.0f);
        m_Character.Update(Physics(), dt);
        if (m_PlayerEntity.IsValid())
            if (auto* t = ECS().GetComponent<Transform>(m_PlayerEntity))
                t->Position = m_Character.GetPosition();
    }

    void OnRender() override {
        m_Gizmo.BeginFrame();
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        DrawToolbar();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        DrawDetails();
        m_Content.DrawImGui(Assets(), &Renderer());
        m_UI.DrawConsole();
        m_AI.Draw(Renderer());
        m_Agent.DrawImGui();
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

        Renderer().BeginShadowPass({0, 0, 0}, 40.0f);
        Renderer().RenderAllShadowCascades(drawScene);

        u32 vw = m_UI.GetDesiredViewportWidth();
        u32 vh = m_UI.GetDesiredViewportHeight();
        Renderer().EnsureSceneRT(vw, vh);
        Renderer().BeginSceneRT();
        drawScene();
        Renderer().EndSceneRT();

        m_UI.DrawViewport(Renderer(), Renderer().GetSceneRTGpuHandle(),
                          Renderer().GetSceneRTWidth(), Renderer().GetSceneRTHeight());

        if (!m_PIE.IsPlaying())
            DrawViewportGizmo();

        m_UI.RenderDrawData();
        m_UI.EndFrame();
    }

    void OnShutdown() override {
        m_Character.Destroy(Physics());
        m_Audio.Shutdown();
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

    void DrawToolbar() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Toolbar");
        if (m_PIE.IsPlaying()) {
            if (ImGui::Button("Stop (F5)")) m_PIE.Stop(ECS(), &m_Character);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 1, 0.3f, 1), "PLAYING");
        } else {
            if (ImGui::Button("Play (F5)")) m_PIE.Play(ECS(), &m_Character);
        }
        ImGui::SameLine();
        if (ImGui::Button("Undo")) m_Undo.Undo(ECS());
        ImGui::SameLine();
        if (ImGui::Button("Redo")) m_Undo.Redo(ECS());
        ImGui::SameLine();
        if (ImGui::Button("Save Scene"))
            SceneSerializer::SaveWorld(ECS(), "Assets/Scenes/scene.json", &m_Entities);
        ImGui::SameLine();
        if (ImGui::Button("Load Scene")) {
            m_Entities.clear();
            SceneSerializer::LoadWorld(ECS(), "Assets/Scenes/scene.json", &m_Entities);
        }
        ImGui::Text("v0.7 · AI Builder · Ctrl+S save");
        ImGui::End();
#endif
    }

    void DrawDetails() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Details");
        if (m_Selected.IsValid() && !m_PIE.IsPlaying()) {
            ImGui::Text("Entity %u", m_Selected.GetID());
            Reflection::DrawImGui(ECS(), m_Selected);
            if (ImGui::RadioButton("Translate", m_Gizmo.GetOperation() == GizmoOp::Translate))
                m_Gizmo.SetOperation(GizmoOp::Translate);
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", m_Gizmo.GetOperation() == GizmoOp::Rotate))
                m_Gizmo.SetOperation(GizmoOp::Rotate);
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", m_Gizmo.GetOperation() == GizmoOp::Scale))
                m_Gizmo.SetOperation(GizmoOp::Scale);
        } else if (m_PIE.IsPlaying()) {
            ImGui::TextDisabled("Details locked during Play");
        }
        ImGui::Separator();
        ImGui::Text("Grounded: %s", m_Character.IsGrounded() ? "yes" : "no");
        ImGui::Text("Audio: %s", m_Audio.IsReady() ? "on" : "off");
        ImGui::Text("Shadows: per-pixel cascades");
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
        if (ImGui::IsKeyPressed(ImGuiKey_T)) m_Gizmo.SetOperation(GizmoOp::Translate);
        if (ImGui::IsKeyPressed(ImGuiKey_R)) m_Gizmo.SetOperation(GizmoOp::Rotate);
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) m_Gizmo.SetOperation(GizmoOp::Scale);
        Transform before = *t;
        bool wasUsing = m_Gizmo.IsUsing();
        Mat4 view = Renderer().GetViewMatrix();
        Mat4 proj = Renderer().GetProjectionMatrix();
        m_Gizmo.Draw(view.m, proj.m, pos.x + min.x, pos.y + min.y, size.x, size.y, *t);
        if (!wasUsing && m_Gizmo.IsUsing()) m_Undo.BeginTransformEdit(m_Selected, before);
        if (wasUsing && !m_Gizmo.IsUsing()) m_Undo.EndTransformEdit(m_Selected, *t, ECS());
        ImGui::End();
#endif
    }

    void DrawStats() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Stats");
        auto& p = Profiler::Get();
        ImGui::Text("FPS: %.1f", p.Fps());
        ImGui::Text("Agent: %s", m_Agent.GetStatus().c_str());
        ImGui::End();
#endif
    }

    EditorUI m_UI;
    AIControlPanel m_AI;
    AIClient m_AgentClient;
    AIGameAgent m_Agent;
    ContentBrowser m_Content;
    AudioSystem m_Audio;
    AnimationSystem m_Anim;
    ViewportGizmo m_Gizmo;
    UndoStack m_Undo;
    PlayInEditor m_PIE;
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
