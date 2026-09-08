#pragma once

#include "Core/Core.h"
#include <string>
#include <vector>

namespace Muk {

struct FreeModelOption {
    const char* Id;
    const char* Label;
    bool IsFree;
};

/**
 * BYOK settings — keys never hardcoded.
 * Defaults prefer FREE models on OpenRouter / NVIDIA.
 */
struct UserSettings {
    std::string Provider = "openrouter"; // openrouter | nvidia | openai | custom

    std::string OpenRouterApiKey;
    // Free model (rate-limited). See openrouter.ai/collections/free-models
    std::string OpenRouterModel = "nvidia/nemotron-3.5-lightning:free";
    std::string OpenRouterBaseUrl = "https://openrouter.ai/api/v1";

    std::string NvidiaApiKey;
    // Hosted free-endpoint style model id on integrate.api.nvidia.com
    std::string NvidiaModel = "meta/llama-3.1-8b-instruct";
    std::string NvidiaBaseUrl = "https://integrate.api.nvidia.com/v1";

    std::string OpenAIApiKey;
    std::string OpenAIBaseUrl = "https://api.openai.com/v1";
    std::string OpenAIModel = "gpt-4o-mini";

    std::string CustomApiKey;
    std::string CustomBaseUrl;
    std::string CustomModel;

    std::string ActiveApiKey() const;
    std::string ActiveBaseUrl() const;
    std::string ActiveModel() const;
    bool HasAnyKey() const;

    bool Load();
    bool Save() const;

    static std::string SettingsPath();

    // Curated free / free-tier model lists for the UI
    static const std::vector<FreeModelOption>& OpenRouterFreeModels();
    static const std::vector<FreeModelOption>& NvidiaFreeModels();
};

} // namespace Muk
