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
        // Try local fallback
        in.open("config/settings.ini");
        if (!in) {
            MUK_CORE_INFO("No settings.ini yet — create one at {0}", path.c_str());
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

    MUK_CORE_INFO("Loaded settings (provider={0}, key={1})",
                  Provider.c_str(), HasAnyKey() ? "set" : "missing");
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

    out << "# Muk Game Engine — user settings (BYOK)\n";
    out << "# Keep this file private. Never commit API keys.\n\n";
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
