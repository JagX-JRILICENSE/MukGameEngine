#include "MultiAgentTeam.h"
#include "Core/Log.h"

namespace Muk {

void MultiAgentTeam::ConfigureFromSettings(const UserSettings& settings) {
    m_Settings = settings;
    if (!m_Slots.empty()) return;

    // Default team: prefer dual when both keys present
    bool hasOR = !settings.OpenRouterApiKey.empty();
    bool hasNV = !settings.NvidiaApiKey.empty();

    AgentSlot arch;
    arch.Role = AgentRole::Architect;
    arch.Label = "Architect";
    arch.Provider = hasOR ? "openrouter" : (hasNV ? "nvidia" : settings.Provider);
    arch.Model = hasOR ? settings.OpenRouterModel : settings.NvidiaModel;

    AgentSlot builder;
    builder.Role = AgentRole::Builder;
    builder.Label = "Builder";
    builder.Provider = hasNV ? "nvidia" : (hasOR ? "openrouter" : settings.Provider);
    builder.Model = hasNV ? settings.NvidiaModel : settings.OpenRouterModel;

    AgentSlot scripter;
    scripter.Role = AgentRole::Scripter;
    scripter.Label = "Scripter";
    scripter.Provider = hasOR ? "openrouter" : builder.Provider;
    scripter.Model = hasOR ? settings.OpenRouterModel : builder.Model;

    AgentSlot critic;
    critic.Role = AgentRole::Critic;
    critic.Label = "Critic";
    critic.Provider = hasNV ? "nvidia" : arch.Provider;
    critic.Model = hasNV ? settings.NvidiaModel : arch.Model;

    m_Slots = { arch, builder, scripter, critic };
}

void MultiAgentTeam::EnableDualProviderDefaults() {
    m_Slots.clear();
    ConfigureFromSettings(m_Settings);
}

bool MultiAgentTeam::HasWorkingKey() const {
    return m_Settings.HasAnyKey() ||
           !m_Settings.OpenRouterApiKey.empty() ||
           !m_Settings.NvidiaApiKey.empty() ||
           !m_Settings.OpenAIApiKey.empty();
}

bool MultiAgentTeam::HasDualProviders() const {
    return !m_Settings.OpenRouterApiKey.empty() && !m_Settings.NvidiaApiKey.empty();
}

UserSettings MultiAgentTeam::SettingsForSlot(const AgentSlot& slot) const {
    UserSettings s = m_Settings;
    std::string prov = slot.Provider;
    if (prov == "active" || prov.empty())
        prov = m_Settings.Provider;

    s.Provider = prov;
    if (!slot.Model.empty()) {
        if (prov == "nvidia") s.NvidiaModel = slot.Model;
        else if (prov == "openai") s.OpenAIModel = slot.Model;
        else if (prov == "custom") s.CustomModel = slot.Model;
        else s.OpenRouterModel = slot.Model;
    }

    // If requested provider has no key, fall back to any available
    if (s.ActiveApiKey().empty()) {
        if (!m_Settings.OpenRouterApiKey.empty()) {
            s.Provider = "openrouter";
        } else if (!m_Settings.NvidiaApiKey.empty()) {
            s.Provider = "nvidia";
        } else if (!m_Settings.OpenAIApiKey.empty()) {
            s.Provider = "openai";
        }
    }
    return s;
}

AIResponse MultiAgentTeam::AskRole(AgentRole role, const std::string& systemPrompt,
                                   const std::string& userPrompt, float temperature) {
    const AgentSlot* slot = nullptr;
    for (auto& s : m_Slots) {
        if (s.Role == role && s.Enabled) { slot = &s; break; }
    }
    if (!slot && !m_Slots.empty()) slot = &m_Slots[0];
    if (!slot) {
        AIResponse r; r.Error = "No agent slots configured"; return r;
    }

    UserSettings s = SettingsForSlot(*slot);
    m_Client.SetSettings(s);

    MUK_CORE_INFO("Team[{0}] provider={1} model={2}",
                  slot->Label.c_str(), s.Provider.c_str(), s.ActiveModel().c_str());

    std::vector<AIMessage> msgs;
    if (!systemPrompt.empty())
        msgs.push_back({ "system", systemPrompt });
    msgs.push_back({ "user", userPrompt });
    return m_Client.Chat(msgs, temperature);
}

} // namespace Muk
