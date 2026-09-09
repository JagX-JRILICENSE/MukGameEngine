#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "EditorUI/ViewportGizmo.h"
#include "Editor/UndoStack.h"
#include "Editor/PlayInEditor.h"
#include "Editor/Screenshot.h"
#include "Editor/FeatureHub.h"
#include "Renderer/FrustumCulling.h"
#include "AI/AIControlPanel.h"
#include "AI/AIGameAgent.h"
#include "AI/AITools.h"
#include "AI/UserSettings.h"
#include "Asset/ContentBrowser.h"
#include "Scene/SceneSerializer.h"
#include "Scene/Prefab.h"
#include "Reflection/Reflection.h"
#include "Audio/AudioSystem.h"
#include "Animation/Skeleton.h"
#include "Animation/Skinning.h"
#include "Animation/SkinGPU.h"
#include "Navigation/GridPath.h"
#include "Renderer/PostSettings.h"
#include "Renderer/IBLCubemap.h"
#include "Physics/CharacterController.h"
#include "Gameplay/Collectible.h"
#include "Gameplay/TriggerVolume.h"
#include "Particles/ParticleSystem.h"
#include "Core/Profiler.h"
#include "Core/ProjectSettings.h"
#include "Core/ConsoleCommands.h"
#include "Core/GameTimer.h"
#include "Core/StringTable.h"
#include "RHI/DX12/DX12RHI.h"
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

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
        m_Prefabs.RegisterDefaults();
        m_Strings.LoadDefaults();
        m_Project.Load();
        m_IBL.Generate(m_Post);

        m_Content.SetRoot("Assets");
        m_Content.Rescan();

        m_Nav.Configure(20, 20, 1.0f, { -10, 0, -10 });
        m_Nav.BlockWorldCircle({ 1.5f, 0, 0 }, 0.8f);

        RegisterConsole();

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
            m_Hub.Select.Set(e);
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
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Enemy";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = { -4, 0.5f, -4 }; t.Scale = { 0.45f, 0.9f, 0.45f };
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
            Track(e, "Enemy"); m_EnemyEntity = e;
            m_EnemyPath = m_Nav.FindPath(t.Position, { 0, 0, 2 });
        }
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "SkinnedArm";
            auto& t = ECS().AddComponent<Transform>(e); t.Position = { 3, 1.0f, 0 };
            auto& anim = ECS().AddComponent<Animator>(e);
            anim.SkeletonName = "DemoArm"; anim.ClipName = "Wave"; anim.Playing = true;
            Track(e, "SkinnedArm"); m_SkinnedEntity = e;
        }

        TriggerSystem::SpawnBox(ECS(), { 0, 0.5f, 5 }, { 1.5f, 1.0f, 1.5f }, "goal", "GoalZone");
        m_Triggers.SetOnEnter([&](Entity, Entity other, const std::string& tag) {
            if (tag == "goal" && other.GetID() == m_PlayerEntity.GetID()) {
                m_UI.Log(m_Strings.Get("ui.win"));
                m_Particles.Burst({ 0, 1, 5 }, 40, { 0.3f, 1, 0.5f });
                m_Hub.Stats.Event("goal_reached");
            }
        });

        CollectibleSystem::SpawnOrb(ECS(), { -2, 0.4f, 0 }, 1, &m_Entities);
        CollectibleSystem::SpawnOrb(ECS(), { 2, 0.4f, 1 }, 1, &m_Entities);
        CollectibleSystem::SpawnOrb(ECS(), { 0, 0.4f, -2 }, 1, &m_Entities);

        m_Collect.SetOnCollect([&](Entity, int, int total) {
            m_UI.Log(m_Strings.Get("ui.score") + "=" + std::to_string(total));
            m_Hub.Stats.Event("collect", std::to_string(total));
        });
        m_Collect.SetOnComplete([&](int) {
            m_Particles.Burst({ 0, 1, 0 }, 48, { 0.2f, 1.0f, 0.4f });
        });

        m_Timers.Schedule("welcome", 1.0f, [&]() { m_UI.Log("Muk v0.13 ready"); });

        m_Hub.AnimSM.BuildLocomotionDemo();
        m_Hub.Foliage.Scatter(60, 18.f);
        m_Hub.Rebind.SetDefault();
        m_Hub.Partition.Configure(32.f, 1);
        m_Hub.Partition.RegisterChunk(0, 0, "Assets/Scenes/chunk_0_0.mukscene");
        m_Hub.Partition.RegisterChunk(1, 0, "Assets/Scenes/chunk_1_0.mukscene");
        m_Hub.Terrain.Generate(33, 40.f, 1.5f);
        m_Hub.Nav.Bake(m_Nav);
        m_Hub.Stats.Event("session_start", "v0.13");

        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);

        m_UI.Log("v0.13 Feature Hub — cull/LOD/post/nav/cine/net");
    }

    void OnUpdate(float dt) override {
#ifdef MUK_PLATFORM_WINDOWS
        static bool zWas=false,yWas=false,pWas=false,sWas=false,f12Was=false,f9Was=false,dWas=false;
        bool zDown = (GetAsyncKeyState('Z') & 0x8000) != 0;
        bool yDown = (GetAsyncKeyState('Y') & 0x8000) != 0;
        bool dDown = (GetAsyncKeyState('D') & 0x8000) != 0;
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool pDown = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        bool sDown = (GetAsyncKeyState('S') & 0x8000) != 0;
        bool f12 = (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
        bool f9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;

        if (ctrl && zDown && !zWas && !m_PIE.IsPlaying()) m_Undo.Undo(ECS());
        if (ctrl && yDown && !yWas && !m_PIE.IsPlaying()) m_Undo.Redo(ECS());
        if (ctrl && dDown && !dWas && !m_PIE.IsPlaying()) DuplicateSelected();
        if (pDown && !pWas) TogglePIE();
        if (ctrl && sDown && !sWas)
            SceneSerializer::SaveWorld(ECS(), "Assets/Scenes/autosave.mukscene", &m_Entities);
        if (f12 && !f12Was) {
            Screenshot::CaptureSceneRT(Renderer(), "viewport");
            m_Content.Rescan();
        }
        if (f9 && !f9Was) HotReloadScript();
        zWas=zDown; yWas=yDown; pWas=pDown; sWas=sDown; f12Was=f12; f9Was=f9; dWas=dDown;

        if (GetAsyncKeyState('Q') & 0x8000) AITools::OrbitCamera(Renderer(), -40.f * dt, 0);
        if (GetAsyncKeyState('E') & 0x8000) AITools::OrbitCamera(Renderer(),  40.f * dt, 0);
#endif

        static float keyTimer = 0;
        keyTimer += dt;
        if (keyTimer > 2.0f) {
            keyTimer = 0;
            UserSettings s; s.Load(); m_Agent.SetSettings(s);
            m_Agent.SetUndoStack(&m_Undo);
        }

        m_Project.TickDayNight(dt);
        m_Timers.Update(dt);

        m_Agent.Tick(ECS(), Renderer(), &m_Audio, m_Entities, m_Selected, &m_Particles);
        if (m_Agent.Runtime().IsPlaying())
            m_Agent.Runtime().Update(dt);

        m_Collect.Update(ECS(), &m_Particles, &m_Audio);
        m_Triggers.Update(ECS());
        m_Particles.Update(dt);

        if (m_EnemyEntity.IsValid() && m_PlayerEntity.IsValid()) {
            auto* et = ECS().GetComponent<Transform>(m_EnemyEntity);
            auto* pt = ECS().GetComponent<Transform>(m_PlayerEntity);
            if (et && pt) {
                m_PathRepathTimer -= dt;
                if (m_PathRepathTimer <= 0.f || m_EnemyPath.empty()) {
                    m_EnemyPath = m_Hub.Nav.FindPath(m_Nav, et->Position, pt->Position);
                    m_PathRepathTimer = 0.5f;
                }
                et->Position = GridPathfinder::FollowPath(m_EnemyPath, et->Position, 2.5f, dt);
            }
        }

        if (m_SkinnedEntity.IsValid()) {
            if (auto* anim = ECS().GetComponent<Animator>(m_SkinnedEntity)) {
                m_Anim.Update(*anim, dt);
                FillSkinCB(m_SkinCB, *anim);
            }
        }

        m_Audio.SetListener({ Renderer().GetCamera().Eye, {0,0,1}, {0,1,0} });
        m_Audio.Update(dt);

        Vec3 wish{};
#ifdef MUK_PLATFORM_WINDOWS
        if (GetAsyncKeyState('W') & 0x8000) wish.z += 1;
        if (GetAsyncKeyState('S') & 0x8000 && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) wish.z -= 1;
        if (GetAsyncKeyState('A') & 0x8000) wish.x -= 1;
        if (GetAsyncKeyState('D') & 0x8000 && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) wish.x += 1;
        if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && m_PIE.IsPlaying()) m_Character.Jump(6.0f);
#endif
        if (!m_Agent.Runtime().IsPlaying() || m_PIE.IsPlaying()) {
            m_Character.SetMoveInput(wish, m_Project.CameraSpeed);
            m_Character.Update(Physics(), dt);
            if (m_PlayerEntity.IsValid())
                if (auto* t = ECS().GetComponent<Transform>(m_PlayerEntity))
                    t->Position = m_Character.GetPosition();
        }

        m_Hub.Pad.Update(0);
        m_Hub.Wind.Update(dt);
        m_Hub.Water.Update(dt);
        m_Hub.AnimSM.Update(dt);
        m_Hub.Cinema.Update(dt, Renderer());
        m_Hub.Net.Tick(dt);
        if (m_PlayerEntity.IsValid())
            if (auto* pt = ECS().GetComponent<Transform>(m_PlayerEntity)) {
                m_Hub.Net.UpdateLocalPos(pt->Position);
                std::vector<std::string> load, unload;
                m_Hub.Partition.UpdateStreaming(pt->Position, load, unload);
            }
        {
            auto& pad = m_Hub.Pad.State();
            if (pad.Connected) {
                Vec3 gwish{ pad.LeftX, 0, pad.LeftY };
                m_Character.SetMoveInput(gwish, m_Project.CameraSpeed);
            }
        }

        m_FpsHistory[m_FpsHistIdx] = Profiler::Get().Fps();
        m_FpsHistIdx = (m_FpsHistIdx + 1) % 120;
    }

    void OnRender() override {
        m_Gizmo.BeginFrame();
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        DrawToolbar();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        DrawDetails();
        DrawPostPanel();
        DrawProjectPanel();
        DrawPrefabPanel();
        DrawConsolePanel();
        m_Content.DrawImGui(Assets(), &Renderer(), [&](const ContentEntry& e) {
            if (e.Kind == AssetKind::Scene)
                SceneSerializer::LoadWorld(ECS(), e.Path, &m_Entities);
            if (e.Kind == AssetKind::Script) {
                std::ifstream in(e.Path);
                if (in) {
                    std::string src((std::istreambuf_iterator<char>(in)), {});
                    m_Agent.Runtime().SetActiveScript(src);
                    m_UI.Log("Loaded script " + e.Name);
                }
            }
        });
        m_UI.DrawConsole();
        m_AI.Draw(Renderer());
        m_Agent.DrawImGui();
        DrawStats();
        DrawFeatureHub(m_Hub);

        Vec3 amb = m_Post.SampleHemisphere({ 0, 1, 0 });
        if (m_IBL.IsReady()) {
            auto s = m_IBL.SampleDir({ 0, 1, 0 });
            amb = { (amb.x + s.x) * 0.5f, (amb.y + s.y) * 0.5f, (amb.z + s.z) * 0.5f };
        }
        float ambAvg = (amb.x + amb.y + amb.z) / 3.0f;
        float day = m_Project.DayLightFactor();
        float ambient = (0.12f + ambAvg) * day;
        float intensity = 1.3f * m_Post.Exposure * day;
        if (m_Post.BloomEnabled) intensity += m_Post.BloomStrength * 0.15f;

        auto drawScene = [&]() {
            Renderer().SetDirectionalLight({0.45f, -1.0f, 0.35f},
                {1, 0.98f, 0.92f}, intensity, ambient);
            m_Hub.DrawnCount = m_Hub.CulledCount = m_Hub.LOD0 = m_Hub.LOD1 = m_Hub.LOD2 = 0;
            Vec3 eye = Renderer().GetCamera().Eye;
            Mat4 viewM = Renderer().GetViewMatrix();
            Frustum fr = Frustum::FromViewProj(viewM.m);
            ECS().ForEach<Transform, MeshRenderer>([&](Entity e, Transform& t, MeshRenderer& mr) {
                if (!mr.Visible) return;
                if (e.GetID() == m_SkinnedEntity.GetID()) return;
                float dx = t.Position.x-eye.x, dy=t.Position.y-eye.y, dz=t.Position.z-eye.z;
                float dist = std::sqrt(dx*dx+dy*dy+dz*dz);
                float rad = mr.BoundingRadius * std::max(t.Scale.x, 1.f);
                if (m_Hub.FrustumCull && !fr.SphereVisible(t.Position, rad)) {
                    m_Hub.CulledCount++; return;
                }
                int lod = m_Hub.LOD.SelectLOD(dist);
                if (lod == 0) m_Hub.LOD0++; else if (lod==1) m_Hub.LOD1++; else if (lod==2) m_Hub.LOD2++;
                if (lod >= 3) { m_Hub.CulledCount++; return; }
                m_Hub.DrawnCount++;
                Material mat = Material::CreateDefault();
                if (e.GetID() == m_Selected.GetID() || m_Hub.Select.Contains(e))
                    mat.BaseColor = { 1.0f, 0.85f, 0.2f, 1.0f };
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
            if (m_Hub.ShowWater) {
                Material wm = Material::CreateDefault();
                wm.BaseColor = { m_Hub.Water.Color.x, m_Hub.Water.Color.y, m_Hub.Water.Color.z, m_Hub.Water.Opacity };
                Renderer().DrawMesh("Cube", m_Hub.Water.WorldMatrix(), wm);
            }
            if (m_Hub.ShowFoliage) {
                Material fm = Material::CreateUnlit({ 0.2f, 0.7f, 0.25f, 1 });
                for (auto& inst : m_Hub.Foliage.Instances()) {
                    Renderer().DrawMesh("Cube", m_Hub.Foliage.InstanceMatrix(inst, m_Hub.Wind.Phase, m_Hub.Wind.Strength), fm);
                }
            }
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
        m_Project.Save();
        m_Hub.Stats.Save();
        m_Character.Destroy(Physics());
        m_Audio.Shutdown();
        m_UI.Shutdown();
    }

private:
    void Track(Entity e, const std::string& name) {
        m_Entities.push_back({ e, name, false });
        m_UI.Log("Created: " + name);
    }

    void TogglePIE() {
        m_PIE.Toggle(ECS(), &m_Character);
        if (m_PIE.IsPlaying()) m_Hub.Stats.Event("pie_play");
    }

    void HotReloadScript() {
        std::ifstream in("Assets/Scripts/ai_generated.muk");
        if (!in) { m_UI.Log("No ai_generated.muk to reload"); return; }
        std::string src((std::istreambuf_iterator<char>(in)), {});
        m_Agent.Runtime().SetActiveScript(src);
        m_UI.Log("F9 hot-reloaded script");
    }

    void DuplicateSelected() {
        if (!m_Selected.IsValid()) return;
        auto* name = ECS().GetComponent<NameComponent>(m_Selected);
        auto* t = ECS().GetComponent<Transform>(m_Selected);
        auto* mr = ECS().GetComponent<MeshRenderer>(m_Selected);
        if (!t || !mr) return;
        auto e = ECS().CreateEntity();
        std::string n = name ? name->Name + "_Copy" : "Copy";
        ECS().AddComponent<NameComponent>(e).Name = n;
        auto& nt = ECS().AddComponent<Transform>(e);
        nt = *t;
        nt.Position.x += m_Project.GridSnap;
        if (m_Project.SnapEnabled) nt.Position = m_Project.SnapVec(nt.Position);
        auto& nmr = ECS().AddComponent<MeshRenderer>(e);
        nmr = *mr;
        Track(e, n);
        m_Selected = e;
        m_Hub.Select.Add(e);
    }

    void RegisterConsole() {
        m_Console.Register("help", [&](const std::vector<std::string>&) {
            return "spawn_prefab | save | load | fps | time | snap | host | foliage";
        }, "list");
        m_Console.Register("fps", [&](const std::vector<std::string>&) {
            return "FPS=" + std::to_string(Profiler::Get().Fps());
        }, "fps");
        m_Console.Register("save", [&](const std::vector<std::string>&) {
            SceneSerializer::SaveWorld(ECS(), "Assets/Scenes/scene.mukscene", &m_Entities);
            return "saved";
        }, "save");
        m_Console.Register("load", [&](const std::vector<std::string>&) {
            m_Entities.clear();
            SceneSerializer::LoadWorld(ECS(), "Assets/Scenes/scene.mukscene", &m_Entities);
            return "loaded";
        }, "load");
        m_Console.Register("spawn_prefab", [&](const std::vector<std::string>& args) {
            if (args.empty()) return std::string("usage: spawn_prefab Orb");
            auto e = m_Prefabs.Spawn(ECS(), args[0], { 0, 0.5f, 0 }, &m_Entities);
            return e.IsValid() ? "spawned" : "unknown";
        }, "prefab");
        m_Console.Register("time", [&](const std::vector<std::string>& args) {
            if (!args.empty()) m_Project.TimeOfDay = (float)atof(args[0].c_str());
            return "time=" + std::to_string(m_Project.TimeOfDay);
        }, "time");
        m_Console.Register("host", [&](const std::vector<std::string>&) {
            m_Hub.Net.Host("Player1"); return "hosting";
        }, "net host");
        m_Console.Register("foliage", [&](const std::vector<std::string>&) {
            m_Hub.Foliage.Scatter(80, 20.f); return "scattered";
        }, "scatter");
    }

    void DrawToolbar() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Toolbar");
        if (m_PIE.IsPlaying()) {
            if (ImGui::Button(m_Strings.Get("ui.stop").c_str())) m_PIE.Stop(ECS(), &m_Character);
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f,1,0.3f,1), "PLAYING");
        } else {
            if (ImGui::Button(m_Strings.Get("ui.play").c_str())) TogglePIE();
        }
        ImGui::SameLine();
        if (ImGui::Button("Undo")) m_Undo.Undo(ECS());
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) DuplicateSelected();
        ImGui::SameLine();
        if (ImGui::Button("Frame")) AITools::FrameSelection(Renderer(), ECS(), m_Selected);
        ImGui::SameLine();
        if (ImGui::Button("Cine")) { m_Hub.Cinema.BuildDemoOrbit(); m_Hub.Cinema.Play(); }
        ImGui::Text("v0.13 | %s", m_Project.ProjectName.c_str());
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
        if (ImGui::Button("Regen Cubemap")) m_IBL.Generate(m_Post);
        ImGui::End();
#endif
    }

    void DrawProjectPanel() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Project Settings");
        static char nameBuf[128] = {};
        if (nameBuf[0] == 0) snprintf(nameBuf, sizeof(nameBuf), "%s", m_Project.ProjectName.c_str());
        if (ImGui::InputText("Project", nameBuf, sizeof(nameBuf))) m_Project.ProjectName = nameBuf;
        ImGui::Checkbox("Grid Snap", &m_Project.SnapEnabled);
        ImGui::SliderFloat("Snap Size", &m_Project.GridSnap, 0.05f, 2.f);
        ImGui::Checkbox("Fog", &m_Project.FogEnabled);
        ImGui::Checkbox("Day/Night", &m_Project.DayNightCycle);
        ImGui::SliderFloat("Time of Day", &m_Project.TimeOfDay, 0, 24);
        if (ImGui::Button("Save Project")) m_Project.Save();
        ImGui::End();
#endif
    }

    void DrawPrefabPanel() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Prefabs");
        for (auto& [name, desc] : m_Prefabs.All()) {
            ImGui::PushID(name.c_str());
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Spawn")) {
                Vec3 pos{0, 0.5f, 0};
                if (m_Selected.IsValid())
                    if (auto* t = ECS().GetComponent<Transform>(m_Selected)) pos = t->Position;
                pos.x += 1.0f;
                if (m_Project.SnapEnabled) pos = m_Project.SnapVec(pos);
                auto e = m_Prefabs.Spawn(ECS(), name, pos, &m_Entities);
                if (e.IsValid()) { m_Selected = e; m_Hub.Select.Set(e); }
            }
            ImGui::PopID();
        }
        ImGui::End();
#endif
    }

    void DrawConsolePanel() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Command Console");
        for (auto& h : m_ConsoleHist) ImGui::TextUnformatted(h.c_str());
        if (ImGui::InputText("##cmd", m_ConsoleInput, sizeof(m_ConsoleInput),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string line = m_ConsoleInput;
            m_ConsoleHist.push_back("> " + line);
            m_ConsoleHist.push_back(m_Console.Execute(line));
            m_ConsoleInput[0] = 0;
        }
        ImGui::End();
#endif
    }

    void DrawDetails() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Details");
        if (m_Selected.IsValid() && !m_PIE.IsPlaying()) {
            ImGui::Text("Entity %u", m_Selected.GetID());
            Reflection::DrawImGui(ECS(), m_Selected);
            if (auto* t = ECS().GetComponent<Transform>(m_Selected))
                if (ImGui::Button("Snap Position")) t->Position = m_Project.SnapVec(t->Position);
        }
        ImGui::Text("Path %d | Particles %d | Timers %d",
                    (int)m_EnemyPath.size(), m_Particles.AliveCount(), m_Timers.Count());
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
        if (wasUsing && !m_Gizmo.IsUsing()) {
            if (m_Project.SnapEnabled) t->Position = m_Project.SnapVec(t->Position);
            m_Undo.EndTransformEdit(m_Selected, *t, ECS());
        }
        ImGui::End();
#endif
    }

    void DrawStats() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", Profiler::Get().Fps());
        ImGui::PlotLines("##fps", m_FpsHistory, 120, m_FpsHistIdx, nullptr, 0, 120, ImVec2(0, 40));
        ImGui::Text("Agent: %s", m_Agent.GetStatus().c_str());
        ImGui::Text("Draw %d Cull %d", m_Hub.DrawnCount, m_Hub.CulledCount);
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
    TriggerSystem m_Triggers;
    ParticleSystem m_Particles;
    FeatureHubState m_Hub;
    GridPathfinder m_Nav;
    PostSettings m_Post;
    IBLCubemap m_IBL;
    PrefabRegistry m_Prefabs;
    ProjectSettings m_Project;
    ConsoleCommands m_Console;
    GameTimer m_Timers;
    StringTable m_Strings;
    SkinCBData m_SkinCB{};
    char m_ConsoleInput[256] = {};
    std::vector<std::string> m_ConsoleHist;
    float m_FpsHistory[120] = {};
    int m_FpsHistIdx = 0;
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
