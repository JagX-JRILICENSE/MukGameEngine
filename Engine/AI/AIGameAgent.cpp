#include "AIGameAgent.h"
#include "Renderer/Renderer.h"
#include "Audio/AudioSystem.h"
#include "EditorUI/EditorUI.h"
#include "ECS/Component.h"
#include "Gameplay/Collectible.h"
#include "Particles/ParticleSystem.h"
#include "Asset/ContentBrowser.h"
#include "Editor/UndoStack.h"
#include "Core/Log.h"
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

static std::string TrimCopy(const std::string& s) {
    size_t i = s.find_first_not_of(" \t\r\n");
    if (i == std::string::npos) return {};
    size_t j = s.find_last_not_of(" \t\r\n");
    return s.substr(i, j - i + 1);
}

void AIGameAgent::SetClient(AIClient*) {}

void AIGameAgent::SetSettings(const UserSettings& settings) {
    m_Team.SetSettings(settings);
    m_Team.EnableDualProviderDefaults();
    m_Async.SetTeam(&m_Team);
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
    m_Status = "Queueing Architect (async)…";
    m_WaitingAsync = false;
    m_FixAttempts = 0;
    m_LastIssues.clear();
    m_DesignDoc.clear();
    m_SceneActions.clear();
    m_ScriptSource.clear();
    m_UIActions.clear();
    m_Runtime.Clear();
    if (m_Undo) m_Undo->BeginAISpawnBatch();
    Log(std::string("=== Async full build === ") + userBrief);
}

void AIGameAgent::Cancel() {
    m_Phase = AgentPhase::Idle;
    m_WaitingAsync = false;
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

std::string AIGameAgent::ExtractScript(const std::string& text) const {
    auto block = ExtractBlock(text, "BEGIN_SCRIPT", "END_SCRIPT");
    if (!block.empty()) return TrimCopy(block);
    std::istringstream iss(text);
    std::string line, out;
    bool any = false;
    while (std::getline(iss, line)) {
        auto t = TrimCopy(line);
        if (t.rfind("on_start", 0) == 0 || t.rfind("on_update", 0) == 0 ||
            t.rfind("on_trigger", 0) == 0 || t.rfind("proximity", 0) == 0 ||
            t.rfind("set ", 0) == 0 || t.rfind("if ", 0) == 0 || t.rfind("spawn_at", 0) == 0) {
            out += t + "\n"; any = true;
        } else if (any && !t.empty() && t.find("ACTION:") == std::string::npos) {
            out += t + "\n";
        }
    }
    return out;
}

void AIGameAgent::SubmitPhase(AgentRole role, const std::string& tag,
                              const std::string& system, const std::string& user, float temp) {
    AsyncAIRequest req;
    req.Role = role; req.Tag = tag; req.System = system; req.User = user; req.Temperature = temp;
    m_Async.Submit(req);
    m_WaitingAsync = true;
    Log(std::string("Submitted async: ") + tag);
}

void AIGameAgent::ApplyActions(World& world, Renderer& renderer, AudioSystem* audio,
                               std::vector<EditorEntityInfo>& entities, Entity& selected,
                               const std::string& text, ParticleSystem* particles) {
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("ACTION:") == std::string::npos && line.find("LEVEL:") == std::string::npos) continue;

        if (line.find("define_level") != std::string::npos) {
            char id[64] = {}, name[128] = {};
            auto idPos = line.find("id="); auto namePos = line.find("name=");
            if (idPos != std::string::npos) sscanf(line.c_str() + idPos, "id=%63s", id);
            if (namePos != std::string::npos) sscanf(line.c_str() + namePos, "name=%127s", name);
            if (id[0]) {
                GameRuntime::Level L; L.Id = id; L.Name = name[0] ? name : id;
                m_Runtime.AddOrReplaceLevel(L);
                Log(std::string("level defined: ") + id);
            }
            continue;
        }

        if (line.find("spawn_orb") != std::string::npos) {
            float x = 0, y = 0.5f, z = 0;
            if (auto p = line.find("x="); p != std::string::npos) sscanf(line.c_str() + p, "x=%f", &x);
            if (auto p = line.find("y="); p != std::string::npos) sscanf(line.c_str() + p, "y=%f", &y);
            if (auto p = line.find("z="); p != std::string::npos) sscanf(line.c_str() + p, "z=%f", &z);
            auto orb = CollectibleSystem::SpawnOrb(world, { x, y, z }, 1, &entities);
            if (m_Undo && orb.IsValid())
                m_Undo->RecordAISpawn(orb, entities.empty() ? "Orb" : entities.back().Name);
            if (particles) particles->Burst({ x, y, z }, 8, { 1, 0.9f, 0.3f }, 1.5f, 0.3f);
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
            auto& tr = world.AddComponent<Transform>(e);
            tr.Position = { x, y, z }; tr.Scale = { sx, sy, sz };
            auto& mr = world.AddComponent<MeshRenderer>(e);
            mr.MeshName = mesh; mr.MaterialName = mat;

            std::string low = name;
            for (auto& c : low) c = (char)tolower((unsigned char)c);
            if (low.find("orb") != std::string::npos) {
                auto& col = world.AddComponent<CollectibleComponent>(e);
                col.Radius = 1.25f; col.ScoreValue = 1;
                tr.Scale = { 0.35f, 0.35f, 0.35f };
            }
            if (low.find("player") != std::string::npos) {
                auto& coll = world.AddComponent<CollectorComponent>(e);
                coll.TargetScore = 3;
            }

            entities.push_back({ e, name, false });
            selected = e;
            if (m_Undo) m_Undo->RecordAISpawn(e, name);
            Log(std::string("spawn ") + name);
            if (audio) { AudioSourceDesc d; d.ClipName = "place"; d.Position = tr.Position; audio->Play(d); }
            continue;
        }

        if (line.find("set_camera") != std::string::npos) {
            CameraView cam = renderer.GetCamera();
            auto parse = [&](const char* key, Vec3& out) {
                auto p = line.find(key); if (p == std::string::npos) return;
                p = line.find('=', p); float a,b,c;
                if (sscanf(line.c_str() + p + 1, "%f,%f,%f", &a, &b, &c) == 3) out = { a,b,c };
            };
            parse("eye", cam.Eye); parse("target", cam.Target);
            renderer.SetCamera(cam);
            continue;
        }

        if (line.find("set_light") != std::string::npos) {
            float intensity = 1.2f; Vec3 dir{ 0.45f, -1, 0.35f };
            if (auto p = line.find("dir="); p != std::string::npos) {
                float a,b,c; if (sscanf(line.c_str() + p + 4, "%f,%f,%f", &a, &b, &c) == 3) dir = { a,b,c };
            }
            if (auto p = line.find("intensity="); p != std::string::npos)
                sscanf(line.c_str() + p, "intensity=%f", &intensity);
            renderer.SetDirectionalLight(dir, { 1, 0.98f, 0.92f }, intensity, 0.18f);
            continue;
        }

        if (line.find("select") != std::string::npos) {
            char nbuf[64] = {};
            if (auto p = line.find("name="); p != std::string::npos) sscanf(line.c_str() + p, "name=%63s", nbuf);
            for (auto& info : entities) if (info.Name == nbuf) { selected = info.Handle; break; }
            continue;
        }

        if (line.find("focus_camera") != std::string::npos) {
            if (selected.IsValid())
                if (auto* t = world.GetComponent<Transform>(selected))
                    renderer.SetCamera({ { t->Position.x + 4, t->Position.y + 3, t->Position.z - 6 }, t->Position });
            continue;
        }

        if (line.find("play_sound") != std::string::npos && audio) {
            char clip[32] = "beep";
            if (auto p = line.find("name="); p != std::string::npos) sscanf(line.c_str() + p, "name=%31s", clip);
            AudioSourceDesc d; d.ClipName = clip; d.Spatial = false; audio->Play(d);
            continue;
        }

        if (line.find("set_material") != std::string::npos) {
            char nbuf[64] = {}, mat[64] = {};
            if (auto p = line.find("name="); p != std::string::npos) sscanf(line.c_str() + p, "name=%63s", nbuf);
            if (auto p = line.find("material="); p != std::string::npos) sscanf(line.c_str() + p, "material=%63s", mat);
            for (auto& info : entities)
                if (info.Name == nbuf)
                    if (auto* mr = world.GetComponent<MeshRenderer>(info.Handle))
                        mr->MaterialName = mat;
            continue;
        }

        if (line.find("show_ui") != std::string::npos) m_UIActions += line + "\n";
    }
}

ScriptHostCallbacks AIGameAgent::MakeHost(World& world, Renderer& renderer, AudioSystem* audio,
                                          std::vector<EditorEntityInfo>& entities, Entity& selected,
                                          ParticleSystem* particles) {
    ScriptHostCallbacks host;
    host.Spawn = [&](const std::string& name, float x, float y, float z,
                     float sx, float sy, float sz, const std::string& mesh) {
        auto e = world.CreateEntity();
        world.AddComponent<NameComponent>(e).Name = name;
        auto& t = world.AddComponent<Transform>(e);
        t.Position = { x, y, z }; t.Scale = { sx, sy, sz };
        world.AddComponent<MeshRenderer>(e).MeshName = mesh.empty() ? "Cube" : mesh;
        entities.push_back({ e, name, false }); selected = e;
        if (m_Undo) m_Undo->RecordAISpawn(e, name);
    };
    host.DestroyByName = [&](const std::string& name) {
        for (size_t i = 0; i < entities.size(); ++i)
            if (entities[i].Name == name) {
                world.DestroyEntity(entities[i].Handle);
                entities.erase(entities.begin() + (std::ptrdiff_t)i); break;
            }
    };
    host.MoveByName = [&](const std::string& name, float x, float y, float z) {
        for (auto& info : entities)
            if (info.Name == name)
                if (auto* t = world.GetComponent<Transform>(info.Handle)) {
                    t->Position.x += x; t->Position.y += y; t->Position.z += z;
                }
    };
    host.LoadLevel = [&](const std::string& levelId) {
        if (auto* L = m_Runtime.GetLevel(levelId)) {
            if (!L->SceneActions.empty())
                ApplyActions(world, renderer, audio, entities, selected, L->SceneActions, particles);
            if (!L->ScriptSource.empty()) m_Runtime.SetActiveScript(L->ScriptSource);
        }
    };
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
                if (auto* t = world.GetComponent<Transform>(info.Handle)) return t->Position;
        return Vec3{};
    };
    return host;
}

void AIGameAgent::RunLocalVerify(World& world) {
    int meshes = 0; bool hasPlayer = false;
    world.ForEach<MeshRenderer, NameComponent>([&](Entity, MeshRenderer&, NameComponent& n) {
        ++meshes;
        std::string low = n.Name;
        for (auto& c : low) c = (char)tolower((unsigned char)c);
        if (low.find("player") != std::string::npos) hasPlayer = true;
    });
    std::ostringstream issues;
    if (meshes < 2) issues << "ISSUE: too few mesh entities\n";
    if ((m_Brief.find("player") != std::string::npos || m_Brief.find("Player") != std::string::npos) && !hasPlayer)
        issues << "ISSUE: missing Player\n";
    if (m_ScriptSource.empty()) issues << "ISSUE: missing gameplay script\n";
    m_LastIssues = issues.str();
}

bool AIGameAgent::SaveGeneratedAssets(ContentBrowser* browser) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories("Assets/Scripts", ec);
    fs::create_directories("Assets/Scenes", ec);
    bool ok = true;
    if (!m_ScriptSource.empty()) {
        std::ofstream out("Assets/Scripts/ai_generated.muk");
        if (out) { out << m_ScriptSource; Log("Saved Assets/Scripts/ai_generated.muk"); }
        else ok = false;
    }
    if (!m_SceneActions.empty()) {
        std::ofstream out("Assets/Scenes/ai_level_actions.txt");
        if (out) out << m_SceneActions;
    }
    if (!m_DesignDoc.empty()) {
        std::ofstream out("Assets/Scenes/ai_design.txt");
        if (out) out << m_DesignDoc;
    }
    for (auto& L : m_Runtime.Levels()) {
        if (L.ScriptSource.empty()) continue;
        std::ofstream out("Assets/Scripts/level_" + L.Id + ".muk");
        if (out) out << L.ScriptSource;
    }
    if (browser) browser->Rescan();
    return ok;
}

void AIGameAgent::HandleAsyncResults(World& world, Renderer& renderer, AudioSystem* audio,
                                     std::vector<EditorEntityInfo>& entities, Entity& selected,
                                     ParticleSystem* particles) {
    auto results = m_Async.Poll();
    if (results.empty()) return;
    m_WaitingAsync = m_Async.IsBusy();

    for (auto& r : results) {
        if (!r.Response.Success) {
            m_Phase = AgentPhase::Failed; m_Status = r.Response.Error;
            if (m_Undo) m_Undo->EndAISpawnBatch(&entities);
            Log(r.Response.Error, true); m_WaitingAsync = false; return;
        }
        const std::string& content = r.Response.Content;

        if (r.Tag == "architect") {
            m_DesignDoc = content;
            ApplyActions(world, renderer, audio, entities, selected, content, particles);
            Log("Architect done (async)"); m_Phase = AgentPhase::BuildingScene; m_WaitingAsync = false;
        } else if (r.Tag == "builder") {
            m_SceneActions = content;
            ApplyActions(world, renderer, audio, entities, selected, content, particles);
            if (m_Runtime.Levels().empty()) {
                GameRuntime::Level L; L.Id = "level1"; L.Name = "Main"; L.SceneActions = m_SceneActions;
                m_Runtime.AddOrReplaceLevel(L);
            } else if (auto* L = m_Runtime.GetLevel(m_Runtime.Levels()[0].Id)) {
                L->SceneActions = m_SceneActions;
            }
            Log("Builder done (async)"); m_Phase = AgentPhase::WritingScript; m_WaitingAsync = false;
        } else if (r.Tag == "scripter") {
            m_ScriptSource = ExtractScript(content);
            if (m_ScriptSource.empty()) m_ScriptSource = content;
            m_Runtime.SetActiveScript(m_ScriptSource);
            if (!m_Runtime.Levels().empty())
                if (auto* L = m_Runtime.GetLevel(m_Runtime.Levels()[0].Id))
                    L->ScriptSource = m_ScriptSource;
            SaveGeneratedAssets(nullptr);
            Log("Scripter done — autosaved"); m_Phase = AgentPhase::BuildingUI; m_WaitingAsync = false;
        } else if (r.Tag == "critic") {
            Log(std::string("Critic: ") + content.substr(0, 300));
            if (content.find("ISSUE:") != std::string::npos) m_LastIssues += content + "\n";
            if (m_LastIssues.empty()) {
                m_Phase = AgentPhase::Playtesting; m_PlayTestTimer = 3.0f; m_Status = "Playtesting…";
                m_Runtime.StartPlay(MakeHost(world, renderer, audio, entities, selected, particles));
            } else m_Phase = AgentPhase::Fixing;
            m_WaitingAsync = false;
        } else if (r.Tag == "fixer") {
            ApplyActions(world, renderer, audio, entities, selected, content, particles);
            auto sc = ExtractScript(content);
            if (!sc.empty()) { m_ScriptSource = sc; m_Runtime.SetActiveScript(sc); }
            SaveGeneratedAssets(nullptr);
            m_LastIssues.clear(); m_Phase = AgentPhase::Previewing; m_WaitingAsync = false;
        }
    }
}

void AIGameAgent::Tick(World& world, Renderer& renderer, AudioSystem* audio,
                       std::vector<EditorEntityInfo>& entities, Entity& selected,
                       ParticleSystem* particles) {
    if (m_Phase == AgentPhase::Idle || m_Phase == AgentPhase::Done || m_Phase == AgentPhase::Failed)
        return;

    HandleAsyncResults(world, renderer, audio, entities, selected, particles);
    if (m_WaitingAsync) {
        m_Status = m_Async.IsBusy() ? "Waiting AI worker…" : "Waiting AI…";
        return;
    }

    if (m_Phase == AgentPhase::Playtesting) {
        m_Runtime.Update(0.016f);
        m_PlayTestTimer -= 0.016f;
        if (m_PlayTestTimer <= 0 || m_Runtime.HasWon() || m_Runtime.HasLost()) {
            m_Runtime.StopPlay();
            SaveGeneratedAssets(nullptr);
            m_Phase = AgentPhase::Done;
            m_Status = m_Runtime.HasWon() ? "Done — WIN" : m_Runtime.HasLost() ? "Done — LOSE" : "Done";
            if (m_Undo) m_Undo->EndAISpawnBatch(&entities);
            Log(m_Status);
            if (audio) { AudioSourceDesc d; d.ClipName = "success"; d.Spatial = false; audio->Play(d); }
            if (particles) particles->Burst({ 0, 1, 0 }, 40, { 0.3f, 1, 0.4f });
        }
        return;
    }

    if (m_Phase == AgentPhase::Architecting) {
        SubmitPhase(AgentRole::Architect, "architect",
            "Architect for Muk Engine. Output DESIGN, LEVELS, WIN, CONTROLS, ACTION: define_level id= name=.",
            m_Brief, 0.4f);
        return;
    }
    if (m_Phase == AgentPhase::BuildingScene) {
        SubmitPhase(AgentRole::Builder, "builder",
            "Builder: ONLY ACTION lines. spawn_cube, spawn_orb, set_camera, set_light. Include Floor, Player, Orb1-3.",
            "Brief:\n" + m_Brief + "\nDesign:\n" + m_DesignDoc, 0.3f);
        return;
    }
    if (m_Phase == AgentPhase::WritingScript) {
        SubmitPhase(AgentRole::Scripter, "scripter",
            "Write BEGIN_SCRIPT...END_SCRIPT with WASD, proximity pickups, score, win at 3.",
            "Brief:\n" + m_Brief + "\nScene:\n" + m_SceneActions, 0.25f);
        return;
    }
    if (m_Phase == AgentPhase::BuildingUI) {
        if (m_UIActions.empty()) m_UIActions = "ACTION: show_ui id=title text=Muk Game\n";
        m_Phase = AgentPhase::Previewing; return;
    }
    if (m_Phase == AgentPhase::Previewing) {
        CameraView cam = renderer.GetCamera();
        cam.Eye = { 8, 6, -12 }; cam.Target = { 0, 0.5f, 0 };
        renderer.SetCamera(cam);
        if (audio) { AudioSourceDesc d; d.ClipName = "beep"; d.Spatial = false; audio->Play(d); }
        m_Phase = AgentPhase::Verifying; return;
    }
    if (m_Phase == AgentPhase::Verifying) {
        RunLocalVerify(world);
        std::ostringstream user;
        user << "Brief: " << m_Brief << "\nScript:\n" << m_ScriptSource.substr(0, 2000) << "\nEntities:\n";
        world.ForEach<NameComponent, Transform>([&](Entity, NameComponent& n, Transform&) {
            user << "- " << n.Name << "\n";
        });
        SubmitPhase(AgentRole::Critic, "critic",
            "Reply OK or ISSUE: lines. Need Player, orbs, movement script, win condition.",
            user.str(), 0.15f);
        return;
    }
    if (m_Phase == AgentPhase::Fixing) {
        if (m_FixAttempts >= kMaxFixAttempts) {
            m_Phase = AgentPhase::Failed; m_Status = "Failed after fix attempts";
            if (m_Undo) m_Undo->EndAISpawnBatch(&entities);
            Log(m_Status, true); return;
        }
        ++m_FixAttempts;
        SubmitPhase(AgentRole::Builder, "fixer",
            "Fix issues. Output ACTIONs and/or BEGIN_SCRIPT...END_SCRIPT.",
            m_LastIssues + "\n" + m_Brief, 0.3f);
        return;
    }
}

void AIGameAgent::DrawImGui() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("AI Game Builder");
    ImGui::TextWrapped("Non-blocking AsyncAI. Ctrl+Z undoes last AI spawn batch.");
    if (m_Async.IsBusy())
        ImGui::TextColored(ImVec4(1, 0.85f, 0.2f, 1), "AI worker busy — editor responsive");
    ImGui::InputTextMultiline("##brief", m_BriefEdit, sizeof(m_BriefEdit), ImVec2(-1, 70));
    if (!IsBusy()) {
        if (ImGui::Button("Build complete game with AI")) StartBuild(m_BriefEdit);
        ImGui::SameLine();
        if (ImGui::Button("Save assets now")) SaveGeneratedAssets(nullptr);
    } else if (ImGui::Button("Cancel")) Cancel();
    ImGui::TextWrapped("%s", m_Status.c_str());
    if (ImGui::CollapsingHeader("Gameplay script"))
        ImGui::TextUnformatted(m_ScriptSource.empty() ? "(none)" : m_ScriptSource.c_str());
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
