#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "EditorUI/ViewportGizmo.h"
#include "Editor/UndoStack.h"
#include "Editor/PlayInEditor.h"
#include "Editor/Screenshot.h"
#include "AI/AIControlPanel.h"
#include "AI/AIGameAgent.h"
#include "AI/UserSettings.h"
#include "Asset/ContentBrowser.h"
#include "Scene/SceneSerializer.h"
#include "Reflection/Reflection.h"
#include "Audio/AudioSystem.h"
#include "Animation/Skeleton.h"
#include "Animation/Skinning.h"
#include "Navigation/GridPath.h"
#include "Renderer/PostSettings.h"
#include "Physics/CharacterController.h"
#include "Gameplay/Collectible.h"
#include "Particles/ParticleSystem.h"
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

        UserSettings s; s.Load();
        m_Agent.SetSettings(s);
        m_Agent.SetUndoStack(&m_Undo);

        m_Audio.Initialize();
        m_Anim.EnsureDemoAssets();
        m_Content.SetRoot("Assets");
        m_Content.Rescan();

        // Nav grid 20x20 cells, 1m each, origin -10,-10
        m_Nav.Configure(20, 20, 1.0f, { -10, 0, -10 });
        m_Nav.BlockWorldCircle({ 1.5f, 0, 0 }, 0.8f); // cube obstacle

        Renderer().SetCamera({ {0.0f, 3.0f, -8.0f}, {0.0f, 0.5f, 0.0f} });

        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Sun";
            auto& L = ECS().AddComponent<DirectionalLight>(e);
            L.Direction = {0.45f, -1.0f, 0.35f}; L.Intensity = 1.5f; L.Ambient = 0.18f;
            Track(e, "Sun");
        }
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Floor";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {0, -0.1f, 0}; t.Scale = {12, 0.2f, 12};
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
            Track(e, "Floor");
        }
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Cube";
            auto& t = ECS().AddComponent<Transform>(e); t.Position = {1.5f, 0.5f, 0};
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
            Track(e, "Cube"); m_Selected = e;
        }
        {
            CharacterDesc cd; cd.Position = {0, 1.0f, 2};
            m_Character.Create(Physics(), cd);
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Player";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = cd.Position; t.Scale = {0.5f, 1.0f, 0.5f};
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
            auto& coll = ECS().AddComponent<CollectorComponent>(e);
            coll.TargetScore = 3;
            Track(e, "Player"); m_PlayerEntity = e;
        }
        // Patrol enemy uses pathfinding
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Enemy";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = { -4, 0.5f, -4 }; t.Scale = { 0.45f, 0.9f, 0.45f };
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
            Track(e, "Enemy"); m_EnemyEntity = e;
            m_EnemyPath = m_Nav.FindPath(t.Position, { 0, 0, 2 });
        }
        // Skinned demo arm
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "SkinnedArm";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = { 3, 1.0f, 0 };
            auto& anim = ECS().AddComponent<Animator>(e);
            anim.SkeletonName = "DemoArm";
            anim.ClipName = "Wave";
            anim.Playing = true;
            Track(e, "SkinnedArm"); m_SkinnedEntity = e;
        }

        CollectibleSystem::SpawnOrb(ECS(), { -2, 0.4f, 0 }, 1, &m_Entities);
        CollectibleSystem::SpawnOrb(ECS(), { 2, 0.4f, 1 }, 1, &m_Entities);
        CollectibleSystem::SpawnOrb(ECS(), { 0, 0.4f, -2 }, 1, &m_Entities);

        m_Collect.SetOnCollect([&](Entity, int, int total) {
            m_UI.Log("Collected! score=" + std::to_string(total));
        });
        m_Collect.SetOnComplete([&](int total) {
            m_UI.Log("All orbs collected score=" + std::to_string(total));
            m_Particles.Burst({ 0, 1, 0 }, 48, { 0.2f, 1.0f, 0.4f });
        });

        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);

        m_UI.Log("v0.11 icon · AI undo · pathfinding · bloom/IBL · skinning");
    }

    void OnUpdate(float dt) override {
#ifdef MUK_PLATFORM_WINDOWS
        static bool zWas=false,yWas=false,pWas=false,sWas=false,f12Was=false;
        bool zDown = (GetAsyncKeyState('Z') & 0x8000) != 0;
        bool yDown = (GetAsyncKeyState('Y') & 0x8000) != 0;
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool pDown = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        bool sDown = (GetAsyncKeyState('S') & 0x8000) != 0;
        bool f12 = (GetAsyncKeyState(VK_F12) & 0x8000) != 0;

        if (ctrl && zDown && !zWas && !m_PIE.IsPlaying()) m_Undo.Undo(ECS());
        if (ctrl && yDown && !yWas && !m_PIE.IsPlaying()) m_Undo.Redo(ECS());
        if (pDown && !pWas) m_PIE.Toggle(ECS(), &m_Character);
        if (ctrl && sDown && !sWas)
            SceneSerializer::SaveWorld(ECS(), "Assets/Scenes/autosave.json", &m_Entities);
        if (f12 && !f12Was) {
            Screenshot::CaptureSceneRT(Renderer(), "viewport");
            m_Content.Rescan();
        }
        zWas=zDown; yWas=yDown; pWas=pDown; sWas=sDown; f12Was=f12;
#endif

        static float keyTimer = 0;
        keyTimer += dt;
        if (keyTimer > 2.0f) {
            keyTimer = 0;
            UserSettings s; s.Load(); m_Agent.SetSettings(s);
            m_Agent.SetUndoStack(&m_Undo);
        }

        m_Agent.Tick(ECS(), Renderer(), &m_Audio, m_Entities, m_Selected, &m_Particles);
        if (m_Agent.Runtime().IsPlaying() && m_Agent.GetPhase() == AgentPhase::Done)
            m_Agent.Runtime().Update(dt);

        m_Collect.Update(ECS(), &m_Particles, &m_Audio);
        m_Particles.Update(dt);

        // Pathfinding: enemy chases player
        if (m_EnemyEntity.IsValid() && m_PlayerEntity.IsValid()) {
            auto* et = ECS().GetComponent<Transform>(m_EnemyEntity);
            auto* pt = ECS().GetComponent<Transform>(m_PlayerEntity);
            if (et && pt) {
                m_PathRepathTimer -= dt;
                if (m_PathRepathTimer <= 0.f || m_EnemyPath.empty()) {
                    m_EnemyPath = m_Nav.FindPath(et->Position, pt->Position);
                    m_PathRepathTimer = 0.5f;
                }
                et->Position = GridPathfinder::FollowPath(m_EnemyPath, et->Position, 2.5f, dt);
            }
        }

        // Skinning update
        if (m_SkinnedEntity.IsValid()) {
            if (auto* anim = ECS().GetComponent<Animator>(m_SkinnedEntity))
                m_Anim.Update(*anim, dt);
        }

        m_Audio.SetListener({ Renderer().GetCamera().Eye, {0,0,1}, {0,1,0} });
        m_Audio.Update(dt);

        Vec3 wish{};
#ifdef MUK_PLATFORM_WINDOWS
        if (GetAsyncKeyState('W') & 0x8000) wish.z += 1;
        if (GetAsyncKeyState('S') & 0x8000) wish.z -= 1;
        if (GetAsyncKeyState('A') & 0x8000) wish.x -= 1;
        if (GetAsyncKeyState('D') & 0x8000) wish.x += 1;
        if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && m_PIE.IsPlaying()) m_Character.Jump(6.0f);
#endif
        if (!m_Agent.Runtime().IsPlaying()) {
            m_Character.SetMoveInput(wish, 5.0f);
            m_Character.Update(Physics(), dt);
            if (m_PlayerEntity.IsValid())
                if (auto* t = ECS().GetComponent<Transform>(m_PlayerEntity))
                    t->Position = m_Character.GetPosition();
        }
    }

    void OnRender() override {
        m_Gizmo.BeginFrame();
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        DrawToolbar();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        DrawDetails();
        DrawPostPanel();
        m_Content.DrawImGui(Assets(), &Renderer());
        m_UI.DrawConsole();
        m_AI.Draw(Renderer());
        m_Agent.DrawImGui();
        DrawStats();

        // Ambient from hemisphere IBL
        Vec3 amb = m_Post.SampleHemisphere({ 0, 1, 0 });
        float ambAvg = (amb.x + amb.y + amb.z) / 3.0f;
        float ambient = 0.12f + ambAvg;
        float intensity = 1.3f * m_Post.Exposure;
        if (m_Post.BloomEnabled)
            intensity += m_Post.BloomStrength * 0.15f;

        auto drawScene = [&]() {
            Renderer().SetDirectionalLight({0.45f, -1.0f, 0.35f},
                {1, 0.98f, 0.92f}, intensity, ambient);
            ECS().ForEach<Transform, MeshRenderer>([&](Entity e, Transform& t, MeshRenderer& mr) {
                if (!mr.Visible) return;
                if (e.GetID() == m_SkinnedEntity.GetID()) return; // drawn via skinning
                Material mat = Material::CreateDefault();
                // Cheap bloom: lift albedo slightly when bloom on
                if (m_Post.BloomEnabled) {
                    mat.BaseColor.x = std::min(1.f, mat.BaseColor.x + m_Post.BloomStrength * 0.05f);
                    mat.BaseColor.y = std::min(1.f, mat.BaseColor.y + m_Post.BloomStrength * 0.05f);
                    mat.BaseColor.z = std::min(1.f, mat.BaseColor.z + m_Post.BloomStrength * 0.08f);
                }
                if (auto m = Assets().GetMaterial(mr.MaterialName)) mat = *m;
                Renderer().DrawMesh(mr.MeshName, t.GetMatrix(), mat);
            });
            if (m_SkinnedEntity.IsValid()) {
                auto* anim = ECS().GetComponent<Animator>(m_SkinnedEntity);
                auto* t = ECS().GetComponent<Transform>(m_SkinnedEntity);
                if (anim && t) {
                    Material sm = Material::CreateUnlit({ 0.9f, 0.7f, 0.3f, 1 });
                    SkinningSystem::DrawSkeletonDebug(Renderer(), *anim, t->GetMatrix(), sm);
                }
            }
            m_Particles.Render(Renderer());
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

        if (!m_PIE.IsPlaying()) DrawViewportGizmo();

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
        m_Entities.push_back({ e, name, false });
        m_UI.Log("Created: " + name);
    }

    void DrawToolbar() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Toolbar");
        if (m_PIE.IsPlaying()) {
            if (ImGui::Button("Stop (F5)")) m_PIE.Stop(ECS(), &m_Character);
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f,1,0.3f,1), "PLAYING");
        } else {
            if (ImGui::Button("Play (F5)")) m_PIE.Play(ECS(), &m_Character);
        }
        ImGui::SameLine();
        if (ImGui::Button("Undo AI")) m_Undo.Undo(ECS());
        ImGui::SameLine();
        if (ImGui::Button("Screenshot (F12)")) {
            Screenshot::CaptureSceneRT(Renderer(), "viewport");
            m_Content.Rescan();
        }
        ImGui::Text("v0.11 · %s", m_Undo.CanUndo() ? m_Undo.PeekUndoName() : "no undo");
        ImGui::End();
#endif
    }

    void DrawPostPanel() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Post / IBL");
        ImGui::SliderFloat("Exposure", &m_Post.Exposure, 0.2f, 3.0f);
        ImGui::Checkbox("Bloom", &m_Post.BloomEnabled);
        ImGui::SliderFloat("Bloom Strength", &m_Post.BloomStrength, 0, 1.5f);
        ImGui::SliderFloat("IBL Strength", &m_Post.IblStrength, 0, 1.0f);
        ImGui::ColorEdit3("Sky", &m_Post.SkyColor.x);
        ImGui::ColorEdit3("Ground", &m_Post.GroundColor.x);
        ImGui::ColorEdit3("Env Spec", &m_Post.EnvSpecular.x);
        ImGui::SliderFloat("Env Spec Str", &m_Post.EnvSpecularStrength, 0, 1);
        ImGui::End();
#endif
    }

    void DrawDetails() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Details");
        if (m_Selected.IsValid() && !m_PIE.IsPlaying()) {
            ImGui::Text("Entity %u", m_Selected.GetID());
            Reflection::DrawImGui(ECS(), m_Selected);
        }
        ImGui::Text("Path waypoints: %d", (int)m_EnemyPath.size());
        ImGui::Text("Particles: %d", m_Particles.AliveCount());
        if (m_PlayerEntity.IsValid())
            if (auto* c = ECS().GetComponent<CollectorComponent>(m_PlayerEntity))
                ImGui::Text("Score: %d / %d", c->Score, c->TargetScore);
        ImGui::End();
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
        ImGui::Text("FPS: %.1f", Profiler::Get().Fps());
        ImGui::Text("Agent: %s", m_Agent.GetStatus().c_str());
        ImGui::End();
#endif
    }

    EditorUI m_UI;
    AIControlPanel m_AI;
    AIGameAgent m_Agent;
    ContentBrowser m_Content;
    AudioSystem m_Audio;
    AnimationSystem m_Anim;
    ViewportGizmo m_Gizmo;
    UndoStack m_Undo;
    PlayInEditor m_PIE;
    CharacterController m_Character;
    CollectibleSystem m_Collect;
    ParticleSystem m_Particles;
    GridPathfinder m_Nav;
    PostSettings m_Post;
    std::vector<Vec3> m_EnemyPath;
    float m_PathRepathTimer = 0;
    std::vector<EditorEntityInfo> m_Entities;
    Entity m_Selected;
    Entity m_PlayerEntity;
    Entity m_EnemyEntity;
    Entity m_SkinnedEntity;
};

int main() {
    EditorApp app;
    app.Run();
    return 0;
}
