#pragma once

#include "AIClient.h"
#include "UserSettings.h"
#include <string>
#include <vector>

namespace Muk {

enum class AgentRole {
    Architect,  // high-level design, levels, win conditions
    Builder,    // ACTION lines + scene
    Scripter,   // gameplay script source
    Critic      // verify + request fixes
};

struct AgentSlot {
    AgentRole Role = AgentRole::Architect;
    std::string Provider; // openrouter | nvidia | openai | active
    std::string Model;    // empty = use provider default from settings
    std::string Label;
    bool Enabled = true;
};

/**
 * Runs multiple LLM roles. Each slot can use OpenRouter, NVIDIA, or the active key.
 * Collaboration: Architect → Builder → Scripter → Critic, with cross-reads of prior output.
 */
class MultiAgentTeam {
public:
    void ConfigureFromSettings(const UserSettings& settings);
    void SetSettings(const UserSettings& settings) { m_Settings = settings; ConfigureFromSettings(settings); }

    // Ensure at least 2 agents when both keys exist
    void EnableDualProviderDefaults();

    const std::vector<AgentSlot>& Slots() const { return m_Slots; }
    std::vector<AgentSlot>& Slots() { return m_Slots; }

    AIResponse AskRole(AgentRole role, const std::string& systemPrompt, const std::string& userPrompt,
                       float temperature = 0.35f);

    bool HasWorkingKey() const;
    bool HasDualProviders() const;

private:
    UserSettings SettingsForSlot(const AgentSlot& slot) const;

    UserSettings m_Settings;
    std::vector<AgentSlot> m_Slots;
    AIClient m_Client;
};

} // namespace Muk
