#include "AIGameAgent.h"
#include "Renderer/Renderer.h"
#include "Audio/AudioSystem.h"
#include "EditorUI/EditorUI.h"
#include "ECS/Component.h"
#include "Core/Log.h"
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

void AIGameAgent::SetClient(AIClient*) {
    // Team owns clients per role; kept for API compat
}

void AIGameAgent::SetSettings(const UserSettings& settings) {
    m_Team.SetSettings(settings);
    m_Team.EnableDualProviderDefaults();
}

void AIGameAgent::Log(const std::string& s, bool err) {
    m_Log.push_back({ s, err });
    if (m_Log.size() > 300) m_Log.erase(m_Log.begin());
    if (err) MUK_CORE_ERROR("{0}", s.c_str());
    else MUK_CORE_INFO("{0}", s.c_str());
}

void AIGameAgent::StartBuild(const std::string& userBrief) {
    if (!m_Team.HasWorkingKey()) {
        Log("No API key — set OpenRouter and/or NVIDIA in AI Control", true);
        m_Phase = AgentPhase::Failed;
        m_Status = "Missing API key";
        return;
    }
    m_Brief = userBrief;
    m_Phase = AgentPhase::Architecting;
    m_Status = "Architecting (multi-agent)…";
    m_FixAttempts = 0;
    m_LastIssues.clear();
    m_DesignDoc.clear();
    m_SceneActions.clear();
    m_ScriptSource.clear();
    m_UIActions.clear();
    m_Runtime.Clear();
    Log(std::string("=== Full build start === ") + userBrief);
    if (m_Team.HasDualProviders())
        Log("Dual providers active: OpenRouter + NVIDIA collaborating");
    else
        Log("Single provider team (add both keys for dual-AI)");
}

void AIGameAgent::Cancel() {
    m_Phase = AgentPhase::Idle;
    m_Status = "Cancelled";
    m_Runtime.StopPlay();
    Log("Cancelled");
}

std::string AIGameAgent::ExtractBlock(const std::string& text, const std::string& beginTag,
                                      const std::string& endTag) const {
    auto a = text.find(beginTag);
    if (a == std::string::npos) return {};
    a += beginTag.size();
    auto b = text.find(endTag, a);
    if (b == std::string::npos) return TrimCopy(text.substr(a));
    return text.substr(a, b - a);
}

// local helper
static std::string TrimCopy(const std::string& s) {
    size_t i = s.find_first_not_of(" \t\r\n");
    if (i == std::string::npos) return {};
    size_t j = s.find_last_not_of(" \t\r\n");
    return s.substr(i, j - i + 1);
}

std::string AIGameAgent::ExtractScript(const std::string& text) const {
    auto block = ExtractBlock(text, "BEGIN_SCRIPT", "END_SCRIPT");
    if (!block.empty()) return TrimCopy(block);
    // fallback: lines that look like script
    std::istringstream iss(text);
    std::string line, out;
    bool any = false;
    while (std::getline(iss, line)) {
        auto t = TrimCopy(line);
        if (t.rfind("on_start", 0) == 0 || t.rfind("on_update", 0) == 0 ||
            t.rfind("on_trigger", 0) == 0 || t.rfind("set ", 0) == 0 ||
            t.rfind("if ", 0) == 0 || t.rfind("spawn_at", 0) == 0) {
            out += t + "\n";
            any = true;
        } else if (any && !t.empty() && t.find("ACTION:") == std::string::npos) {
            out += t + "\n";
        }
    }
    return out;
}

void AIGameAgent::ApplyActions(World& world, Renderer& renderer, AudioSystem* audio,
                               std::vector<EditorEntityInfo>& entities, Entity& selected,
                               const std::string& text) {
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("ACTION:") == std::string::npos &&
            line.find("LEVEL:") == std::string::npos) continue;

        if (line.find("define_level") != std::string::npos) {
            char id[64] = {}, name[128] = {};
            sscanf(line.c_str(), "%*s define_level id=%63s name=%127s", id, name);
            if (id[0]) {
                GameRuntime::Level L;
                L.Id = id;
                L.Name = name[0] ? name : id;
                m_Runtime.AddOrReplaceLevel(L);
                Log(std::string("level defined: ") + id);
            }
            continue;
        }

        if (line.find("spawn_cube") != std::string::npos || line.find("spawn") != std::string::npos) {
            std::string name = "AI_Prop";
            float x = 0, y = 0.5f, z = 0, sx = 1, sy = 1, sz = 1;
            char nbuf[64] = {}, mat[64] = "Default", mesh[64] = "Cube";
            if (auto p = line.find("name="); p != std::string::npos) sscanf(line.c_str() + p, "name=%63s", nbuf);
            if (nbuf[0]) name = nbuf;
            if (auto p = line.find("x="); p != std::string::npos) sscanf(line.c_str() + p, "x=%f", &x);
            if (auto p = line.find("y="); p != std::string::npos) sscanf(line.c_str() + p, "y=%f", &y);
            if (auto p = line.find("z="); p != std::string::npos) sscanf(line.c_str() + p, "z=%f", &z);
            if (auto p = line.find("sx="); p != std::string::npos) sscanf(line.c_str() + p, "sx=%f", &sx);
            if (auto p = line.find("sy="); p != std::string::npos) sscanf(line.c_str() + p, "sy=%f", &sy);
            if (auto p = line.find("sz="); p != std::string::npos) sscanf(line.c_str() + p, "sz=%f", &sz);
            if (auto p = line.find("material="); p != std::string::npos) sscanf(line.c_str() + p, "material=%63s", mat);
            if (auto p = line.find("mesh="); p != std::string::npos) sscanf(line.c_str() + p, "mesh=%63s", mesh);

            auto e = world.CreateEntity();
            world.AddComponent<NameComponent>(e).Name = name;
            auto& t = world.AddComponent<Transform>(e);
            t.Position = { x, y, z };
            t.Scale = { sx, sy, sz };
            auto& mr = world.AddComponent<MeshRenderer>(e);
            mr.MeshName = mesh;
            mr.MaterialName = mat;
            entities.push_back({ e, name, false });
            selected = e;
            Log(std::string("spawn ") + name);
            if (audio) {
                AudioSourceDesc d; d.ClipName = "place"; d.Position = t.Position; audio->Play(d);
            }
            continue;
        }

        if (line.find("set_camera") != std::string::npos) {
            CameraView cam = renderer.GetCamera();
            auto parse = [&](const char* key, Vec3& out) {
                auto p = line.find(key);
                if (p == std::string::npos) return;
                p = line.find('=', p);
                float a, b, c;
                if (sscanf(line.c_str() + p + 1, "%f,%f,%f", &a, &b, &c) == 3) out = { a, b, c };
            };
            parse("eye", cam.Eye);
            parse("target", cam.Target);
            renderer.SetCamera(cam);
            continue;
        }

        if (line.find("set_light") != std::string::npos) {
            float intensity = 1.2f;
            Vec3 dir{ 0.45f, -1, 0.35f };
            if (auto p = line.find("dir="); p != std::string::npos) {
                float a, b, c;
                if (sscanf(line.c_str() + p + 4, "%f,%f,%f", &a, &b, &c) == 3) dir = { a, b, c };
            }
            if (auto p = line.find("intensity="); p != std::string::npos)
                sscanf(line.c_str() + p, "intensity=%f", &intensity);
            renderer.SetDirectionalLight(dir, { 1, 0.98f, 0.92f }, intensity, 0.18f);
            continue;
        }

        if (line.find("select") != std::string::npos) {
            char nbuf[64] = {};
            if (auto p = line.find("name="); p != std::string::npos)
                sscanf(line.c_str() + p, "name=%63s", nbuf);
            for (auto& info : entities)
                if (info.Name == nbuf) { selected = info.Handle; break; }
            continue;
        }

        if (line.find("focus_camera") != std::string::npos) {
            if (selected.IsValid())
                if (auto* t = world.GetComponent<Transform>(selected)) {
                    CameraView cam = renderer.GetCamera();
                    cam.Target = t->Position;
                    cam.Eye = { t->Position.x + 4, t->Position.y + 3, t->Position.z - 6 };
                    renderer.SetCamera(cam);
                }
            continue;
        }

        if (line.find("play_sound") != std::string::npos && audio) {
            char clip[32] = "beep";
            if (auto p = line.find("name="); p != std::string::npos)
                sscanf(line.c_str() + p, "name=%31s", clip);
            AudioSourceDesc d; d.ClipName = clip; d.Spatial = false; audio->Play(d);
            continue;
        }

        if (line.find("set_material") != std::string::npos) {
            char nbuf[64] = {}, mat[64] = {};
            if (auto p = line.find("name="); p != std::string::npos)
                sscanf(line.c_str() + p, "name=%63s", nbuf);
            if (auto p = line.find("material="); p != std::string::npos)
                sscanf(line.c_str() + p, "material=%63s", mat);
            for (auto& info : entities)
                if (info.Name == nbuf)
                    if (auto* mr = world.GetComponent<MeshRenderer>(info.Handle))
                        mr->MaterialName = mat;
            continue;
        }

        if (line.find("show_ui") != std::string::npos) {
            // handled when script/UI phase applies via runtime; also stash
            m_UIActions += line + "\n";
        }
    }
}

ScriptHostCallbacks AIGameAgent::MakeHost(World& world, Renderer& renderer, AudioSystem* audio,
                                          std::vector<EditorEntityInfo>& entities, Entity& selected) {
    ScriptHostCallbacks host;
    host.Spawn = [&](const std::string& name, float x, float y, float z,
                     float sx, float sy, float sz, const std::string& mesh) {
        auto e = world.CreateEntity();
        world.AddComponent<NameComponent>(e).Name = name;
        auto& t = world.AddComponent<Transform>(e);
        t.Position = { x, y, z }; t.Scale = { sx, sy, sz };
        auto& mr = world.AddComponent<MeshRenderer>(e);
        mr.MeshName = mesh.empty() ? "Cube" : mesh;
        entities.push_back({ e, name, false });
        selected = e;
    };
    host.DestroyByName = [&](const std::string& name) {
        for (size_t i = 0; i < entities.size(); ++i) {
            if (entities[i].Name == name) {
                world.DestroyEntity(entities[i].Handle);
                entities.erase(entities.begin() + (std::ptrdiff_t)i);
                break;
            }
        }
    };
    host.MoveByName = [&](const std::string& name, float x, float y, float z) {
        for (auto& info : entities) {
            if (info.Name != name) continue;
            if (auto* t = world.GetComponent<Transform>(info.Handle)) {
                t->Position.x += x; t->Position.y += y; t->Position.z += z;
            }
        }
    };
    host.LoadLevel = [&](const std::string& levelId) {
        if (auto* L = m_Runtime.GetLevel(levelId)) {
            Log(std::string("load_level ") + levelId);
            if (!L->SceneActions.empty())
                ApplyActions(world, renderer, audio, entities, selected, L->SceneActions);
            if (!L->ScriptSource.empty())
                m_Runtime.SetActiveScript(L->ScriptSource);
        }
    };
    host.ShowUI = nullptr;
    host.HideUI = nullptr;
    host.PlaySound = [audio](const std::string& clip) {
        if (!audio) return;
        AudioSourceDesc d; d.ClipName = clip; d.Spatial = false; audio->Play(d);
    };
    host.Win = [&](const std::string& msg) { Log(std::string("WIN: ") + msg); };
    host.Lose = [&](const std::string& msg) { Log(std::string("LOSE: ") + msg, true); };
    host.IsKeyDown = [](const std::string& key) -> bool {
#ifdef MUK_PLATFORM_WINDOWS
        if (key == "W" || key == "w") return (GetAsyncKeyState('W') & 0x8000) != 0;
        if (key == "A" || key == "a") return (GetAsyncKeyState('A') & 0x8000) != 0;
        if (key == "S" || key == "s") return (GetAsyncKeyState('S') & 0x8000) != 0;
        if (key == "D" || key == "d") return (GetAsyncKeyState('D') & 0x8000) != 0;
        if (key == "Space" || key == "space") return (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
#else
        (void)key;
#endif
        return false;
    };
    host.GetPos = [&](const std::string& name) {
        for (auto& info : entities)
            if (info.Name == name)
                if (auto* t = world.GetComponent<Transform>(info.Handle))
                    return t->Position;
        return Vec3{};
    };
    return host;
}

void AIGameAgent::RunLocalVerify(World& world) {
    int meshes = 0;
    bool hasPlayer = false;
    world.ForEach<MeshRenderer, NameComponent>([&](Entity, MeshRenderer&, NameComponent& n) {
        ++meshes;
        std::string low = n.Name;
        for (auto& c : low) c = (char)tolower((unsigned char)c);
        if (low.find("player") != std::string::npos) hasPlayer = true;
    });
    std::ostringstream issues;
    if (meshes < 2) issues << "ISSUE: too few mesh entities (" << meshes << ")\n";
    if (m_Brief.find("player") != std::string::npos && !hasPlayer)
        issues << "ISSUE: missing Player entity\n";
    if (m_ScriptSource.empty())
        issues << "ISSUE: missing gameplay script\n";
    if (m_Runtime.Levels().empty() && m_Brief.find("level") != std::string::npos)
        issues << "ISSUE: no levels registered\n";
    m_LastIssues = issues.str();
}

void AIGameAgent::Tick(World& world, Renderer& renderer, AudioSystem* audio,
                       std::vector<EditorEntityInfo>& entities, Entity& selected) {
    if (m_Phase == AgentPhase::Idle || m_Phase == AgentPhase::Done || m_Phase == AgentPhase::Failed)
        return;

    if (m_Phase == AgentPhase::Playtesting) {
        m_Runtime.Update(0.016f);
        m_PlayTestTimer -= 0.016f;
        if (m_PlayTestTimer <= 0 || m_Runtime.HasWon() || m_Runtime.HasLost()) {
            m_Runtime.StopPlay();
            m_Phase = AgentPhase::Done;
            m_Status = m_Runtime.HasWon() ? "Done — playtest WIN" :
                       m_Runtime.HasLost() ? "Done — playtest LOSE" : "Done — full pipeline complete";
            Log(m_Status);
            if (audio) { AudioSourceDesc d; d.ClipName = "success"; d.Spatial = false; audio->Play(d); }
        }
        return;
    }

    if (m_Phase == AgentPhase::Architecting) {
        m_Status = "Architect designing levels & win conditions…";
        std::string sys =
            "You are the Architect for Muk Engine. Design a COMPLETE playable game.\n"
            "Output sections:\n"
            "DESIGN: (paragraph)\n"
            "LEVELS: list id and name\n"
            "WIN: condition\n"
            "CONTROLS: WASD etc\n"
            "Then ACTION lines for level metadata:\n"
            "ACTION: define_level id=level1 name=Arena\n";
        auto resp = m_Team.AskRole(AgentRole::Architect, sys, m_Brief, 0.4f);
        if (!resp.Success) {
            m_Phase = AgentPhase::Failed; m_Status = resp.Error; Log(resp.Error, true); return;
        }
        m_DesignDoc = resp.Content;
        ApplyActions(world, renderer, audio, entities, selected, resp.Content);
        Log("Architect done");
        m_Phase = AgentPhase::BuildingScene;
        return;
    }

    if (m_Phase == AgentPhase::BuildingScene) {
        m_Status = "Builder placing scene…";
        std::string sys =
            "You are the Builder for Muk Engine. Emit ONLY ACTION lines to place the world.\n"
            "Allowed:\n"
            "ACTION: spawn_cube name=X x= y= z= sx= sy= sz= mesh=Cube material=Default\n"
            "ACTION: set_camera eye=x,y,z target=x,y,z\n"
            "ACTION: set_light dir=x,y,z intensity=1.2\n"
            "ACTION: select name=X\n"
            "ACTION: focus_camera\n"
            "Create Floor, pillars/props, Player, collectible Orbs if needed. Bounds -10..10.\n";
        std::string user = "Brief:\n" + m_Brief + "\n\nDesign doc:\n" + m_DesignDoc;
        auto resp = m_Team.AskRole(AgentRole::Builder, sys, user, 0.3f);
        if (!resp.Success) {
            m_Phase = AgentPhase::Failed; m_Status = resp.Error; Log(resp.Error, true); return;
        }
        m_SceneActions = resp.Content;
        ApplyActions(world, renderer, audio, entities, selected, resp.Content);
        // stash into first level
        if (!m_Runtime.Levels().empty()) {
            auto& L = const_cast<GameRuntime::Level&>(m_Runtime.Levels()[0]);
            L.SceneActions = m_SceneActions;
        } else {
            GameRuntime::Level L; L.Id = "level1"; L.Name = "Main"; L.SceneActions = m_SceneActions;
            m_Runtime.AddOrReplaceLevel(L);
        }
        Log("Builder done");
        m_Phase = AgentPhase::WritingScript;
        return;
    }

    if (m_Phase == AgentPhase::WritingScript) {
        m_Status = "Scripter writing gameplay…";
        std::string sys =
            "You are the Scripter for Muk Engine. Write Muk Script gameplay.\n"
            "Wrap code between BEGIN_SCRIPT and END_SCRIPT.\n"
            "Language:\n"
            "  set score 0\n"
            "  on_start\n"
            "    show_ui hud Score:0\n"
            "    show_ui title Collect orbs\n"
            "  on_update dt\n"
            "    if key W then move Player 0 0 5*dt\n"
            "    if key S then move Player 0 0 -5*dt\n"
            "    if key A then move Player -5*dt 0 0\n"
            "    if key D then move Player 5*dt 0 0\n"
            "    if score >= 3 then win You win\n"
            "Commands: set/add, spawn_at, destroy, move, load_level, show_ui, hide_ui, play_sound, win, lose, if..then\n"
            "Entity names must match Builder (Player, Orb1...).\n";
        std::string user = "Brief:\n" + m_Brief + "\nDesign:\n" + m_DesignDoc +
                           "\nScene actions:\n" + m_SceneActions;
        auto resp = m_Team.AskRole(AgentRole::Scripter, sys, user, 0.25f);
        if (!resp.Success) {
            m_Phase = AgentPhase::Failed; m_Status = resp.Error; Log(resp.Error, true); return;
        }
        m_ScriptSource = ExtractScript(resp.Content);
        if (m_ScriptSource.empty()) m_ScriptSource = resp.Content;
        m_Runtime.SetActiveScript(m_ScriptSource);
        if (!m_Runtime.Levels().empty()) {
            // can't mutate const from Levels() - use GetLevel
            if (auto* L = m_Runtime.GetLevel(m_Runtime.Levels()[0].Id))
                L->ScriptSource = m_ScriptSource;
        }
        Log("Scripter done (" + std::to_string(m_ScriptSource.size()) + " chars)");
        m_Phase = AgentPhase::BuildingUI;
        return;
    }

    if (m_Phase == AgentPhase::BuildingUI) {
        m_Status = "UI flow…";
        // Ensure title/hud via script on_start; optional extra ACTIONs
        if (m_UIActions.empty())
            m_UIActions = "ACTION: show_ui id=title text=Muk Game\n";
        Log("UI phase complete");
        m_Phase = AgentPhase::Previewing;
        return;
    }

    if (m_Phase == AgentPhase::Previewing) {
        m_Status = "Preview framing…";
        CameraView cam = renderer.GetCamera();
        cam.Eye = { 8, 6, -12 };
        cam.Target = { 0, 0.5f, 0 };
        renderer.SetCamera(cam);
        if (audio) { AudioSourceDesc d; d.ClipName = "beep"; d.Spatial = false; audio->Play(d); }
        m_Phase = AgentPhase::Verifying;
        return;
    }

    if (m_Phase == AgentPhase::Verifying) {
        m_Status = "Critic verifying…";
        RunLocalVerify(world);
        std::string sys =
            "You are the Critic. Check the game is complete. Reply OK or ISSUE: lines.\n"
            "Require: scene entities, Player, gameplay script with on_update movement, win condition.\n";
        std::ostringstream user;
        user << "Brief: " << m_Brief << "\nDesign: " << m_DesignDoc.substr(0, 1500)
             << "\nScript:\n" << m_ScriptSource.substr(0, 2000) << "\nEntities:\n";
        world.ForEach<NameComponent, Transform>([&](Entity, NameComponent& n, Transform& t) {
            user << "- " << n.Name << " @ " << t.Position.x << "," << t.Position.y << "," << t.Position.z << "\n";
        });
        auto resp = m_Team.AskRole(AgentRole::Critic, sys, user.str(), 0.15f);
        if (resp.Success) {
            Log(std::string("Critic: ") + resp.Content.substr(0, 300));
            if (resp.Content.find("ISSUE:") != std::string::npos)
                m_LastIssues += resp.Content + "\n";
        }
        if (m_LastIssues.empty() && (resp.Success && resp.Content.find("OK") != std::string::npos)) {
            m_Phase = AgentPhase::Playtesting;
            m_PlayTestTimer = 2.5f;
            m_Status = "Playtesting scripts…";
            m_Runtime.StartPlay(MakeHost(world, renderer, audio, entities, selected));
            Log("Playtest started");
            return;
        }
        if (m_LastIssues.empty()) {
            // soft pass
            m_Phase = AgentPhase::Playtesting;
            m_PlayTestTimer = 2.0f;
            m_Runtime.StartPlay(MakeHost(world, renderer, audio, entities, selected));
            return;
        }
        m_Phase = AgentPhase::Fixing;
        return;
    }

    if (m_Phase == AgentPhase::Fixing) {
        if (m_FixAttempts >= kMaxFixAttempts) {
            m_Phase = AgentPhase::Failed;
            m_Status = "Failed after fix attempts — see log";
            Log(m_Status, true);
            return;
        }
        ++m_FixAttempts;
        m_Status = "Fix attempt " + std::to_string(m_FixAttempts);
        std::string sys =
            "Fix the Muk game. Output ACTION lines and/or BEGIN_SCRIPT...END_SCRIPT.\n"
            "Address every ISSUE.";
        std::string user = m_LastIssues + "\nBrief: " + m_Brief +
                           "\nCurrent script:\n" + m_ScriptSource;
        auto resp = m_Team.AskRole(AgentRole::Builder, sys, user, 0.3f);
        if (!resp.Success) {
            m_Phase = AgentPhase::Failed; m_Status = resp.Error; return;
        }
        ApplyActions(world, renderer, audio, entities, selected, resp.Content);
        auto sc = ExtractScript(resp.Content);
        if (!sc.empty()) {
            m_ScriptSource = sc;
            m_Runtime.SetActiveScript(sc);
        }
        m_LastIssues.clear();
        m_Phase = AgentPhase::Previewing;
    }
}

void AIGameAgent::DrawImGui() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("AI Game Builder");
    ImGui::TextWrapped(
        "Multi-agent pipeline (Architect/Builder/Scripter/Critic). "
        "Uses OpenRouter + NVIDIA together when both keys are set. "
        "Writes scenes, levels, Muk Script gameplay, UI, then verifies and playtests.");
    ImGui::Checkbox("Prefer dual providers", &m_UseDual);
    if (m_Team.HasDualProviders())
        ImGui::TextColored(ImVec4(0.3f, 1, 0.4f, 1), "Dual AI: OpenRouter + NVIDIA");
    else
        ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "Add both API keys for dual-agent mode");

    ImGui::InputTextMultiline("##brief", m_BriefEdit, sizeof(m_BriefEdit), ImVec2(-1, 70));
    if (!IsBusy()) {
        if (ImGui::Button("Build complete game with AI"))
            StartBuild(m_BriefEdit);
    } else {
        if (ImGui::Button("Cancel")) Cancel();
    }
    ImGui::SameLine();
    ImGui::TextWrapped("%s", m_Status.c_str());

    if (ImGui::CollapsingHeader("Team roles")) {
        for (auto& s : m_Team.Slots()) {
            ImGui::Text("%s → %s (%s)", s.Label.c_str(), s.Provider.c_str(),
                        s.Model.empty() ? "default" : s.Model.c_str());
        }
    }
    if (ImGui::CollapsingHeader("Gameplay script")) {
        ImGui::TextUnformatted(m_ScriptSource.empty() ? "(none yet)" : m_ScriptSource.c_str());
    }
    if (ImGui::CollapsingHeader("Levels")) {
        for (auto& L : m_Runtime.Levels())
            ImGui::BulletText("%s (%s)", L.Id.c_str(), L.Name.c_str());
    }

    // Runtime HUD
    for (auto& u : m_Runtime.UI())
        if (u.Visible) ImGui::TextColored(ImVec4(0.7f, 0.9f, 1, 1), "[UI %s] %s", u.Id.c_str(), u.Text.c_str());
    if (!m_Runtime.Banner().empty())
        ImGui::TextColored(ImVec4(1, 0.9f, 0.2f, 1), "%s", m_Runtime.Banner().c_str());

    ImGui::Separator();
    ImGui::BeginChild("agentlog", ImVec2(0, 160), true);
    for (auto& l : m_Log) {
        if (l.IsError) ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "%s", l.Text.c_str());
        else ImGui::TextWrapped("%s", l.Text.c_str());
    }
    ImGui::EndChild();
    ImGui::End();
#endif
}

} // namespace Muk
