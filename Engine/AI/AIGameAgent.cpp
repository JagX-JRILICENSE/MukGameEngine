#include "AIGameAgent.h"
#include "Renderer/Renderer.h"
#include "Audio/AudioSystem.h"
#include "EditorUI/EditorUI.h"
#include "ECS/Component.h"
#include "Core/Log.h"
#include <sstream>
#include <cstdio>
#include <cstring>

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

void AIGameAgent::Log(const std::string& s, bool err) {
    m_Log.push_back({ s, err });
    if (m_Log.size() > 200) m_Log.erase(m_Log.begin());
    if (err) MUK_CORE_ERROR("{0}", s.c_str());
    else MUK_CORE_INFO("{0}", s.c_str());
}

void AIGameAgent::StartBuild(const std::string& userBrief) {
    if (!m_Client) { Log("No AI client", true); return; }
    m_Brief = userBrief;
    m_Phase = AgentPhase::Planning;
    m_Status = "Planning layout…";
    m_FixAttempts = 0;
    m_LastIssues.clear();
    Log(std::string("Agent start: ") + userBrief);
}

void AIGameAgent::Cancel() {
    m_Phase = AgentPhase::Idle;
    m_Status = "Cancelled";
    Log("Agent cancelled");
}

std::string AIGameAgent::BuildPlanPrompt() const {
    return std::string(
        "You are Muk AI Game Designer. Design a small playable scene for this brief:\n\""
    ) + m_Brief + "\"\n"
    "Reply ONLY with ACTION lines (no markdown). Allowed actions:\n"
    "ACTION: clear_props\n"
    "ACTION: spawn_cube name=X x= y= z= sx= sy= sz= material=Default|CheckerMat\n"
    "ACTION: set_camera eye=x,y,z target=x,y,z\n"
    "ACTION: set_light dir=x,y,z intensity=1.2\n"
    "ACTION: select name=X\n"
    "ACTION: focus_camera  (look at selected)\n"
    "ACTION: play_sound name=beep|place|success x= y= z=\n"
    "ACTION: set_material name=Entity material=CheckerMat\n"
    "Create floor, props, and a Player. Keep coordinates small (-10..10).\n";
}

std::string AIGameAgent::BuildVerifyPrompt(World& world) const {
    std::ostringstream oss;
    oss << "You verified a Muk scene built for: \"" << m_Brief << "\"\nEntities:\n";
    world.ForEach<NameComponent, Transform>([&](Entity, NameComponent& n, Transform& t) {
        oss << "- " << n.Name << " pos=" << t.Position.x << "," << t.Position.y << "," << t.Position.z << "\n";
    });
    oss << "List problems as lines ISSUE: description. If good, reply exactly: OK\n"
           "Check: has floor, not all objects stacked, camera sensible, player present if requested.\n";
    return oss.str();
}

std::string AIGameAgent::BuildFixPrompt() const {
    return std::string("Fix these scene issues with ACTION lines only:\n") + m_LastIssues +
           "\nBrief was: " + m_Brief + "\nSame ACTION vocabulary as before.\n";
}

void AIGameAgent::ApplyActions(World& world, Renderer& renderer, AudioSystem* audio,
                               std::vector<EditorEntityInfo>& entities, Entity& selected,
                               const std::string& text) {
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("ACTION:") == std::string::npos) continue;

        if (line.find("clear_props") != std::string::npos) {
            // Soft clear: only AI_ / generated names would be ideal; skip destructive clear for safety
            Log("skip clear_props (safe mode)");
            continue;
        }

        if (line.find("spawn_cube") != std::string::npos) {
            std::string name = "AI_Prop";
            float x=0,y=0.5f,z=0,sx=1,sy=1,sz=1;
            char nbuf[64] = {};
            char mat[64] = "Default";
            if (auto p = line.find("name="); p != std::string::npos)
                sscanf(line.c_str() + p, "name=%63s", nbuf);
            if (nbuf[0]) name = nbuf;
            if (auto p = line.find("x="); p != std::string::npos) sscanf(line.c_str()+p, "x=%f", &x);
            if (auto p = line.find("y="); p != std::string::npos) sscanf(line.c_str()+p, "y=%f", &y);
            if (auto p = line.find("z="); p != std::string::npos) sscanf(line.c_str()+p, "z=%f", &z);
            if (auto p = line.find("sx="); p != std::string::npos) sscanf(line.c_str()+p, "sx=%f", &sx);
            if (auto p = line.find("sy="); p != std::string::npos) sscanf(line.c_str()+p, "sy=%f", &sy);
            if (auto p = line.find("sz="); p != std::string::npos) sscanf(line.c_str()+p, "sz=%f", &sz);
            if (auto p = line.find("material="); p != std::string::npos) sscanf(line.c_str()+p, "material=%63s", mat);

            auto e = world.CreateEntity();
            world.AddComponent<NameComponent>(e).Name = name;
            auto& t = world.AddComponent<Transform>(e);
            t.Position = {x,y,z};
            t.Scale = {sx,sy,sz};
            auto& mr = world.AddComponent<MeshRenderer>(e);
            mr.MeshName = "Cube";
            mr.MaterialName = mat;
            entities.push_back({ e, name, false });
            selected = e;
            Log(std::string("spawn ") + name);
            if (audio) {
                AudioSourceDesc d;
                d.ClipName = "place";
                d.Position = t.Position;
                audio->Play(d);
            }
            continue;
        }

        if (line.find("set_camera") != std::string::npos) {
            CameraView cam = renderer.GetCamera();
            auto parse = [&](const char* key, Vec3& out) {
                auto p = line.find(key);
                if (p == std::string::npos) return;
                p = line.find('=', p);
                float a,b,c;
                if (sscanf(line.c_str()+p+1, "%f,%f,%f", &a,&b,&c)==3) out={a,b,c};
            };
            parse("eye", cam.Eye);
            parse("target", cam.Target);
            renderer.SetCamera(cam);
            Log("set_camera");
            continue;
        }

        if (line.find("set_light") != std::string::npos) {
            float intensity = 1.2f;
            Vec3 dir{0.45f,-1,0.35f};
            if (auto p = line.find("dir="); p != std::string::npos) {
                float a,b,c;
                if (sscanf(line.c_str()+p+4, "%f,%f,%f", &a,&b,&c)==3) dir={a,b,c};
            }
            if (auto p = line.find("intensity="); p != std::string::npos)
                sscanf(line.c_str()+p, "intensity=%f", &intensity);
            renderer.SetDirectionalLight(dir, {1,0.98f,0.92f}, intensity, 0.18f);
            Log("set_light");
            continue;
        }

        if (line.find("select") != std::string::npos) {
            char nbuf[64] = {};
            if (auto p = line.find("name="); p != std::string::npos)
                sscanf(line.c_str()+p, "name=%63s", nbuf);
            for (auto& info : entities) {
                if (info.Name == nbuf) {
                    selected = info.Handle;
                    Log(std::string("cursor → ") + nbuf);
                    break;
                }
            }
            continue;
        }

        if (line.find("focus_camera") != std::string::npos) {
            if (selected.IsValid()) {
                if (auto* t = world.GetComponent<Transform>(selected)) {
                    CameraView cam = renderer.GetCamera();
                    cam.Target = t->Position;
                    cam.Eye = { t->Position.x + 4, t->Position.y + 3, t->Position.z - 6 };
                    renderer.SetCamera(cam);
                    Log("focus_camera");
                }
            }
            continue;
        }

        if (line.find("play_sound") != std::string::npos && audio) {
            char clip[32] = "beep";
            float x=0,y=0,z=0;
            if (auto p = line.find("name="); p != std::string::npos)
                sscanf(line.c_str()+p, "name=%31s", clip);
            if (auto p = line.find("x="); p != std::string::npos) sscanf(line.c_str()+p, "x=%f", &x);
            if (auto p = line.find("y="); p != std::string::npos) sscanf(line.c_str()+p, "y=%f", &y);
            if (auto p = line.find("z="); p != std::string::npos) sscanf(line.c_str()+p, "z=%f", &z);
            AudioSourceDesc d; d.ClipName = clip; d.Position = {x,y,z};
            audio->Play(d);
            continue;
        }

        if (line.find("set_material") != std::string::npos) {
            char nbuf[64]={}, mat[64]={};
            if (auto p = line.find("name="); p != std::string::npos)
                sscanf(line.c_str()+p, "name=%63s", nbuf);
            if (auto p = line.find("material="); p != std::string::npos)
                sscanf(line.c_str()+p, "material=%63s", mat);
            for (auto& info : entities) {
                if (info.Name == nbuf) {
                    if (auto* mr = world.GetComponent<MeshRenderer>(info.Handle))
                        mr->MaterialName = mat;
                    Log(std::string("material ") + nbuf);
                }
            }
        }
    }
}

void AIGameAgent::RunVerify(World& world) {
    int meshes = 0;
    bool hasFloor = false;
    world.ForEach<MeshRenderer, NameComponent>([&](Entity, MeshRenderer&, NameComponent& n) {
        ++meshes;
        std::string low = n.Name;
        for (auto& c : low) c = (char)tolower(c);
        if (low.find("floor") != std::string::npos || low.find("ground") != std::string::npos)
            hasFloor = true;
    });
    std::ostringstream issues;
    if (meshes < 2) issues << "ISSUE: too few meshes (" << meshes << ")\n";
    if (!hasFloor && m_Brief.find("floor") != std::string::npos)
        issues << "ISSUE: no floor entity\n";
    m_LastIssues = issues.str();
}

void AIGameAgent::Tick(World& world, Renderer& renderer, AudioSystem* audio,
                       std::vector<EditorEntityInfo>& entities, Entity& selected) {
    if (!m_Client || m_Phase == AgentPhase::Idle || m_Phase == AgentPhase::Done || m_Phase == AgentPhase::Failed)
        return;

    if (m_Phase == AgentPhase::Planning) {
        m_Status = "Calling AI for plan…";
        auto resp = m_Client->Chat({ {"user", BuildPlanPrompt()} }, 0.35f);
        if (!resp.Success) {
            m_Phase = AgentPhase::Failed;
            m_Status = resp.Error;
            Log(resp.Error, true);
            if (audio) { AudioSourceDesc d; d.ClipName = "error"; d.Spatial = false; audio->Play(d); }
            return;
        }
        m_LastPlan = resp.Content;
        Log("Plan received");
        m_Phase = AgentPhase::Building;
        return;
    }

    if (m_Phase == AgentPhase::Building) {
        m_Status = "Building scene from actions…";
        ApplyActions(world, renderer, audio, entities, selected, m_LastPlan);
        m_Phase = AgentPhase::Previewing;
        return;
    }

    if (m_Phase == AgentPhase::Previewing) {
        m_Status = "Preview — framing camera…";
        // Auto frame scene origin
        CameraView cam = renderer.GetCamera();
        cam.Eye = {6, 5, -10};
        cam.Target = {0, 0.5f, 0};
        renderer.SetCamera(cam);
        if (audio) { AudioSourceDesc d; d.ClipName = "beep"; d.Spatial = false; audio->Play(d); }
        m_Phase = AgentPhase::Verifying;
        return;
    }

    if (m_Phase == AgentPhase::Verifying) {
        m_Status = "Verifying design…";
        RunVerify(world);
        auto resp = m_Client->Chat({ {"user", BuildVerifyPrompt(world)} }, 0.2f);
        if (resp.Success) {
            Log(std::string("Verify: ") + resp.Content.substr(0, 200));
            if (resp.Content.find("OK") != std::string::npos && m_LastIssues.empty()) {
                m_Phase = AgentPhase::Done;
                m_Status = "Done — scene ready";
                if (audio) { AudioSourceDesc d; d.ClipName = "success"; d.Spatial = false; audio->Play(d); }
                return;
            }
            if (resp.Content.find("ISSUE:") != std::string::npos)
                m_LastIssues += resp.Content;
        }
        if (m_LastIssues.empty()) {
            m_Phase = AgentPhase::Done;
            m_Status = "Done";
            return;
        }
        m_Phase = AgentPhase::Fixing;
        return;
    }

    if (m_Phase == AgentPhase::Fixing) {
        if (m_FixAttempts >= kMaxFixAttempts) {
            m_Phase = AgentPhase::Failed;
            m_Status = "Gave up after fix attempts";
            Log(m_Status, true);
            return;
        }
        ++m_FixAttempts;
        m_Status = std::string("Fix attempt ") + std::to_string(m_FixAttempts);
        auto resp = m_Client->Chat({ {"user", BuildFixPrompt()} }, 0.3f);
        if (!resp.Success) {
            m_Phase = AgentPhase::Failed;
            m_Status = resp.Error;
            return;
        }
        ApplyActions(world, renderer, audio, entities, selected, resp.Content);
        m_LastIssues.clear();
        m_Phase = AgentPhase::Previewing;
    }
}

void AIGameAgent::DrawImGui() {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("AI Game Builder");
    ImGui::TextWrapped("Describe a game/scene. Agent plans, builds, previews, verifies, and fixes.");
    ImGui::InputTextMultiline("##brief", m_BriefEdit, sizeof(m_BriefEdit), ImVec2(-1, 60));
    if (!IsBusy()) {
        if (ImGui::Button("Build with AI")) StartBuild(m_BriefEdit);
    } else {
        if (ImGui::Button("Cancel")) Cancel();
    }
    ImGui::SameLine();
    ImGui::Text("Phase: %s", m_Status.c_str());
    ImGui::Separator();
    ImGui::BeginChild("agentlog");
    for (auto& l : m_Log) {
        if (l.IsError) ImGui::TextColored(ImVec4(1,0.4f,0.3f,1), "%s", l.Text.c_str());
        else ImGui::TextWrapped("%s", l.Text.c_str());
    }
    ImGui::EndChild();
    ImGui::End();
#endif
}

} // namespace Muk
