#pragma once

#include "UserSettings.h"
#include <string>
#include <vector>
#include <functional>

namespace Muk {

struct AIMessage {
    std::string Role;    // system | user | assistant
    std::string Content;
};

struct AIResponse {
    bool Success = false;
    std::string Content;
    std::string Error;
    std::string RawJson;
};

/**
 * Minimal OpenAI-compatible chat client.
 * Works with OpenRouter, NVIDIA Integrate API, OpenAI, and custom base URLs.
 * Uses WinHTTP on Windows (no extra dependency).
 */
class AIClient {
public:
    void SetSettings(const UserSettings& settings) { m_Settings = settings; }
    const UserSettings& GetSettings() const { return m_Settings; }

    AIResponse Chat(const std::vector<AIMessage>& messages, float temperature = 0.4f);

    // Helper: single user prompt with engine-control system prompt
    AIResponse AskEngineControl(const std::string& userPrompt);

private:
    UserSettings m_Settings;
    std::string BuildRequestBody(const std::vector<AIMessage>& messages, float temperature) const;
    AIResponse HttpPostJson(const std::string& url, const std::string& apiKey, const std::string& body);
};

} // namespace Muk
