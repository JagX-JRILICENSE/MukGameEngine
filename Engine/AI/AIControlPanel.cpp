#include "AIControlPanel.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"
#include "Core/Application.h"
#include "ECS/Component.h"
#include <cstring>
#include <sstream>

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

void AIControlPanel::Initialize() {
    m_Settings.Load();
    // Prefer free defaults if model empty
    if (m_Settings.OpenRouterModel.empty())
        m_Settings.OpenRouterModel = "nvidia/nemotron-3.5-lightning:free";
    if (m_Settings.NvidiaModel.empty())
        m_Settings.NvidiaModel = "meta/llama-3.1-8b-instruct";
    m_Client.SetSettings(m_Settings);
    m_Log.push_back("AI Control (BYOK) — free models listed for OpenRouter + NVIDIA");
    m_Log.push_back("Keys: openrouter.ai/keys | build.nvidia.com (Get API Key)");
    m_Log.push_back("Settings file: " + UserSettings::SettingsPath());
}

void AIControlPanel::ApplySimpleActions(Renderer& renderer, const std::string& reply) {
    std::istringstream iss(reply);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("ACTION: set_camera") != std::string::npos) {
            CameraView cam = renderer.GetCamera();
            auto parseVec = [](const std::string& s, const char* key, Vec3& out) {
                auto p = s.find(key);
                if (p == std::string::npos) return;
                p = s.find('=', p);
                if (p == std::string::npos) return;
                float a=0,b=0,c=0;
                if (sscanf(s.c_str() + p + 1, "%f,%f,%f", &a, &b, &c) == 3)
                    out = {a,b,c};
            };
            parseVec(line, "eye", cam.Eye);
            parseVec(line, "target", cam.Target);
            renderer.SetCamera(cam);
            m_Log.push_back("[Applied] set_camera");
        }
        if (line.find("ACTION: spawn_cube") != std::string::npos) {
            float x=0,y=0.5f,z=0;
            auto p = line.find("x=");
            if (p != std::string::npos) sscanf(line.c_str() + p, "x=%f", &x);
            p = line.find("y=");
            if (p != std::string::npos) sscanf(line.c_str() + p, "y=%f", &y);
            p = line.find("z=");
            if (p != std::string::npos) sscanf(line.c_str() + p, "z=%f", &z);

            auto& world = Application::Get().GetWorld();
            auto e = world.CreateEntity();
            world.AddComponent<NameComponent>(e).Name = "AI_Cube";
            auto& t = world.AddComponent<Transform>(e);
            t.Position = {x, y, z};
            auto& mr = world.AddComponent<MeshRenderer>(e);
            mr.MeshName = "Cube";
            mr.MaterialName = "Default";
            m_Log.push_back("[Applied] spawn_cube");
        }
    }
}

void AIControlPanel::Draw(Renderer& renderer) {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("AI Control (BYOK)");

    ImGui::TextUnformatted("Provider");
    const char* providers[] = { "openrouter", "nvidia", "openai", "custom" };
    int prov = 0;
    if (m_Settings.Provider == "nvidia") prov = 1;
    else if (m_Settings.Provider == "openai") prov = 2;
    else if (m_Settings.Provider == "custom") prov = 3;
    if (ImGui::Combo("##provider", &prov, providers, 4)) {
        m_Settings.Provider = providers[prov];
        m_Client.SetSettings(m_Settings);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1), "FREE models");

    if (m_Settings.Provider == "openrouter") {
        ImGui::TextWrapped("OpenRouter free IDs end with :free (rate limits apply). Key: openrouter.ai/keys");
        const auto& models = UserSettings::OpenRouterFreeModels();
        for (const auto& m : models) {
            bool selected = (m_Settings.OpenRouterModel == m.Id);
            if (ImGui::Selectable(m.Label, selected)) {
                m_Settings.OpenRouterModel = m.Id;
                m_Client.SetSettings(m_Settings);
                m_Log.push_back(std::string("Model → ") + m.Id);
            }
        }
    } else if (m_Settings.Provider == "nvidia") {
        ImGui::TextWrapped("NVIDIA integrate.api.nvidia.com — free/credit tier. Key: build.nvidia.com → Get API Key (nvapi-...)");
        const auto& models = UserSettings::NvidiaFreeModels();
        for (const auto& m : models) {
            bool selected = (m_Settings.NvidiaModel == m.Id);
            if (ImGui::Selectable(m.Label, selected)) {
                m_Settings.NvidiaModel = m.Id;
                m_Client.SetSettings(m_Settings);
                m_Log.push_back(std::string("Model → ") + m.Id);
            }
        }
    } else {
        ImGui::TextDisabled("Select OpenRouter or NVIDIA to see free model lists");
    }

    ImGui::Separator();
    ImGui::Text("API key (saved only on this PC)");
    std::string active = m_Settings.ActiveApiKey();
    if (active.size() > 8)
        ImGui::TextDisabled("Key set: %s...%s", active.substr(0, 4).c_str(), active.substr(active.size() - 4).c_str());
    else if (!active.empty())
        ImGui::TextDisabled("Key set");
    else
        ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "Paste key below → Apply → Save");

    ImGui::InputText("##key", m_KeyEdit, sizeof(m_KeyEdit), ImGuiInputTextFlags_Password);
    if (ImGui::Button("Apply key")) {
        if (m_Settings.Provider == "nvidia") m_Settings.NvidiaApiKey = m_KeyEdit;
        else if (m_Settings.Provider == "openai") m_Settings.OpenAIApiKey = m_KeyEdit;
        else if (m_Settings.Provider == "custom") m_Settings.CustomApiKey = m_KeyEdit;
        else m_Settings.OpenRouterApiKey = m_KeyEdit;
        m_Client.SetSettings(m_Settings);
        m_Log.push_back("Key applied (in memory)");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        m_Log.push_back(m_Settings.Save() ? "Saved to " + UserSettings::SettingsPath() : "Save failed");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload")) {
        m_Settings.Load();
        m_Client.SetSettings(m_Settings);
        m_Log.push_back("Reloaded settings");
    }

    ImGui::Text("Active: %s", m_Settings.ActiveModel().c_str());
    ImGui::InputTextMultiline("##prompt", m_Input, sizeof(m_Input), ImVec2(-1, 70));
    if (ImGui::Button("Send") && !m_Busy && std::strlen(m_Input) > 0) {
        m_Busy = true;
        m_Log.push_back(std::string("You: ") + m_Input);
        auto resp = m_Client.AskEngineControl(m_Input);
        if (resp.Success) {
            m_Log.push_back(std::string("AI: ") + resp.Content);
            ApplySimpleActions(renderer, resp.Content);
        } else {
            m_Log.push_back(std::string("Error: ") + resp.Error);
            if (!resp.RawJson.empty() && resp.RawJson.size() < 500)
                m_Log.push_back(resp.RawJson);
        }
        m_Busy = false;
        m_Input[0] = 0;
    }

    ImGui::Separator();
    ImGui::BeginChild("ailog", ImVec2(0, 0), true);
    for (const auto& line : m_Log)
        ImGui::TextWrapped("%s", line.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::End();
#else
    (void)renderer;
#endif
}

} // namespace Muk
