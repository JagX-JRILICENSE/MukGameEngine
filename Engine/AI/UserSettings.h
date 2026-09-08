#pragma once

#include "Core/Core.h"
#include <string>

namespace Muk {

/**
 * Bring-Your-Own-Key settings.
 * Loaded from %APPDATA%/MukGameEngine/settings.ini (Windows) or ./config/settings.ini
 * Keys are never hardcoded; users paste their own.
 */
struct UserSettings {
    std::string Provider = "openrouter"; // openrouter | nvidia | openai | custom

    std::string OpenRouterApiKey;
    std::string OpenRouterModel = "openai/gpt-4o-mini";
    std::string OpenRouterBaseUrl = "https://openrouter.ai/api/v1";

    std::string NvidiaApiKey;
    std::string NvidiaBaseUrl = "https://integrate.api.nvidia.com/v1";
    std::string NvidiaModel = "meta/llama-3.1-8b-instruct";

    std::string OpenAIApiKey;
    std::string OpenAIBaseUrl = "https://api.openai.com/v1";
    std::string OpenAIModel = "gpt-4o-mini";

    std::string CustomApiKey;
    std::string CustomBaseUrl;
    std::string CustomModel;

    // Active resolved endpoint
    std::string ActiveApiKey() const;
    std::string ActiveBaseUrl() const;
    std::string ActiveModel() const;
    bool HasAnyKey() const;

    bool Load();
    bool Save() const;

    static std::string SettingsPath();
};

} // namespace Muk
