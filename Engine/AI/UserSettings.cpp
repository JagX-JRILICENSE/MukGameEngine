#include "UserSettings.h"
#include "Core/Log.h"

#include <fstream>
#include <sstream>
#include <filesystem>

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>
#include <ShlObj.h>
#endif

namespace Muk {

namespace fs = std::filesystem;

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

const std::vector<FreeModelOption>& UserSettings::OpenRouterFreeModels() {
    static const std::vector<FreeModelOption> k = {
        { "nvidia/nemotron-3.5-lightning:free", "NVIDIA Nemotron 3.5 Lightning (FREE)", true },
        { "nvidia/nemotron-3-ultra-550b-a55b:free", "NVIDIA Nemotron 3 Ultra (FREE)", true },
        { "nvidia/nemotron-3-super-120b-a12b:free", "NVIDIA Nemotron 3 Super (FREE)", true },
        { "google/gemma-4-31b-it:free", "Google Gemma 4 31B (FREE)", true },
        { "google/gemma-4-26b-a4b-it:free", "Google Gemma 4 26B (FREE)", true },
        { "openrouter/free", "OpenRouter Free Models Router (FREE)", true },
        { "liquid/lfm-2.5-2.6b:free", "LiquidAI LFM 2.5 (FREE)", true },
        { "poolside/laguna-s-2.1:free", "Poolside Laguna S 2.1 (FREE)", true },
        { "cohere/north-mini-code:free", "Cohere North Mini Code (FREE)", true },
        { "thinkingmachines/inkling-small:free", "Thinking Machines Inkling Small (FREE)", true },
    };
    return k;
}

const std::vector<FreeModelOption>& UserSettings::NvidiaFreeModels() {
    // Hosted endpoints at build.nvidia.com / integrate.api.nvidia.com (free credits / free endpoints)
    static const std::vector<FreeModelOption> k = {
        { "meta/llama-3.1-8b-instruct", "Meta Llama 3.1 8B (NVIDIA free tier)", true },
        { "meta/llama-3.3-70b-instruct", "Meta Llama 3.3 70B (NVIDIA)", true },
        { "nvidia/llama-3.1-nemotron-70b-instruct", "NVIDIA Nemotron 70B", true },
        { "mistralai/mistral-7b-instruct-v0.3", "Mistral 7B Instruct (NVIDIA)", true },
        { "google/gemma-2-9b-it", "Google Gemma 2 9B (NVIDIA)", true },
        { "qwen/qwen2.5-coder-32b-instruct", "Qwen 2.5 Coder 32B (NVIDIA)", true },
    };
    return k;
}

std::string UserSettings::SettingsPath() {
#ifdef MUK_PLATFORM_WINDOWS
    wchar_t* appdata = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
        fs::path p = fs::path(appdata) / "MukGameEngine" / "settings.ini";
        CoTaskMemFree(appdata);
        return p.string();
    }
#endif
    return "config/settings.ini";
}

std::string UserSettings::ActiveApiKey() const {
    if (Provider == "nvidia") return NvidiaApiKey;
    if (Provider == "openai") return OpenAIApiKey;
    if (Provider == "custom") return CustomApiKey;
    return OpenRouterApiKey;
}

std::string UserSettings::ActiveBaseUrl() const {
    if (Provider == "nvidia") return NvidiaBaseUrl;
    if (Provider == "openai") return OpenAIBaseUrl;
    if (Provider == "custom") return CustomBaseUrl;
    return OpenRouterBaseUrl;
}

std::string UserSettings::ActiveModel() const {
    if (Provider == "nvidia") return NvidiaModel;
    if (Provider == "openai") return OpenAIModel;
    if (Provider == "custom") return CustomModel;
    return OpenRouterModel;
}

bool UserSettings::HasAnyKey() const {
    return !ActiveApiKey().empty();
}

bool UserSettings::Load() {
    const std::string path = SettingsPath();
    std::ifstream in(path);
    if (!in) {
        in.open("config/settings.ini");
        if (!in) {
            MUK_CORE_INFO("No settings.ini yet — {0}", path.c_str());
            return false;
        }
    }

    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));

        if (key == "provider") Provider = val;
        else if (key == "openrouter_api_key") OpenRouterApiKey = val;
        else if (key == "openrouter_model") OpenRouterModel = val;
        else if (key == "openrouter_base_url") OpenRouterBaseUrl = val;
        else if (key == "nvidia_api_key") NvidiaApiKey = val;
        else if (key == "nvidia_base_url") NvidiaBaseUrl = val;
        else if (key == "nvidia_model") NvidiaModel = val;
        else if (key == "openai_api_key") OpenAIApiKey = val;
        else if (key == "openai_base_url") OpenAIBaseUrl = val;
        else if (key == "openai_model") OpenAIModel = val;
        else if (key == "custom_api_key") CustomApiKey = val;
        else if (key == "custom_base_url") CustomBaseUrl = val;
        else if (key == "custom_model") CustomModel = val;
    }

    MUK_CORE_INFO("Loaded settings (provider={0}, model={1}, key={2})",
                  Provider.c_str(), ActiveModel().c_str(), HasAnyKey() ? "set" : "missing");
    return true;
}

bool UserSettings::Save() const {
    const std::string path = SettingsPath();
    try {
        fs::path p(path);
        if (p.has_parent_path())
            fs::create_directories(p.parent_path());
    } catch (...) {}

    std::ofstream out(path);
    if (!out) {
        MUK_CORE_ERROR("Failed to write settings: {0}", path.c_str());
        return false;
    }

    out << "# Muk Game Engine — BYOK (never commit real keys)\n";
    out << "# Free OpenRouter models end with :free\n";
    out << "# NVIDIA key from https://build.nvidia.com (Get API Key)\n\n";
    out << "provider=" << Provider << "\n\n";
    out << "openrouter_api_key=" << OpenRouterApiKey << "\n";
    out << "openrouter_model=" << OpenRouterModel << "\n";
    out << "openrouter_base_url=" << OpenRouterBaseUrl << "\n\n";
    out << "nvidia_api_key=" << NvidiaApiKey << "\n";
    out << "nvidia_base_url=" << NvidiaBaseUrl << "\n";
    out << "nvidia_model=" << NvidiaModel << "\n\n";
    out << "openai_api_key=" << OpenAIApiKey << "\n";
    out << "openai_base_url=" << OpenAIBaseUrl << "\n";
    out << "openai_model=" << OpenAIModel << "\n\n";
    out << "custom_api_key=" << CustomApiKey << "\n";
    out << "custom_base_url=" << CustomBaseUrl << "\n";
    out << "custom_model=" << CustomModel << "\n";
    return true;
}

} // namespace Muk
