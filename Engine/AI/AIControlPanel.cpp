#include "AIControlPanel.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"
#include <cstring>
#include <sstream>

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

void AIControlPanel::Initialize() {
    m_Settings.Load();
    m_Client.SetSettings(m_Settings);
    m_Log.push_back("AI Control ready. Bring your own OpenRouter / NVIDIA / OpenAI / custom key.");
    m_Log.push_back("Settings file: " + UserSettings::SettingsPath());
}

void AIControlPanel::ApplySimpleActions(Renderer& renderer, const std::string& reply) {
    // Parse lines like: ACTION: set_camera eye=0,2,-5 target=0,0,0
    std::istringstream iss(reply);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("ACTION: set_camera") != std::string::npos) {
            CameraView cam = {};
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
    ImGui::Text("API key (stored only on this PC)");
    std::string active = m_Settings.ActiveApiKey();
    if (active.size() > 8)
        ImGui::TextDisabled("Key set: %s...%s", active.substr(0, 4).c_str(), active.substr(active.size() - 4).c_str());
    else if (!active.empty())
        ImGui::TextDisabled("Key set (short)");
    else
        ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "No key — paste below and Save");

    ImGui::InputText("##key", m_KeyEdit, sizeof(m_KeyEdit), ImGuiInputTextFlags_Password);
    if (ImGui::Button("Apply key to provider")) {
        if (m_Settings.Provider == "nvidia") m_Settings.NvidiaApiKey = m_KeyEdit;
        else if (m_Settings.Provider == "openai") m_Settings.OpenAIApiKey = m_KeyEdit;
        else if (m_Settings.Provider == "custom") m_Settings.CustomApiKey = m_KeyEdit;
        else m_Settings.OpenRouterApiKey = m_KeyEdit;
        m_Client.SetSettings(m_Settings);
        m_Log.push_back("Key applied in memory — click Save settings to persist.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save settings")) {
        if (m_Settings.Save())
            m_Log.push_back("Saved to " + UserSettings::SettingsPath());
        else
            m_Log.push_back("Save failed");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload")) {
        m_Settings.Load();
        m_Client.SetSettings(m_Settings);
        m_Log.push_back("Reloaded settings");
    }

    ImGui::Separator();
    ImGui::Text("Model: %s", m_Settings.ActiveModel().c_str());
    ImGui::TextWrapped("Ask the model to help design levels, write systems, or emit ACTION lines to tweak the camera.");

    ImGui::InputTextMultiline("##prompt", m_Input, sizeof(m_Input), ImVec2(-1, 80));
    if (ImGui::Button("Send") && !m_Busy && std::strlen(m_Input) > 0) {
        m_Busy = true;
        m_Log.push_back(std::string("You: ") + m_Input);
        auto resp = m_Client.AskEngineControl(m_Input);
        if (resp.Success) {
            m_Log.push_back(std::string("AI: ") + resp.Content);
            ApplySimpleActions(renderer, resp.Content);
        } else {
            m_Log.push_back(std::string("Error: ") + resp.Error);
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
