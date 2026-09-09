#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace Muk {

/** Simple key→string localization table */
class StringTable {
public:
    void Set(const std::string& key, const std::string& value) { m_Map[key] = value; }

    const std::string& Get(const std::string& key) const {
        auto it = m_Map.find(key);
        if (it != m_Map.end()) return it->second;
        return key; // fallback to key
    }

    void LoadDefaults() {
        Set("ui.play", "Play");
        Set("ui.stop", "Stop");
        Set("ui.score", "Score");
        Set("ui.win", "You Win!");
        Set("ui.lose", "Game Over");
        Set("ui.loading", "Loading…");
        Set("ui.ai_build", "Build complete game with AI");
    }

    bool LoadFile(const std::string& path) {
        std::ifstream in(path);
        if (!in) return false;
        std::string line;
        while (std::getline(in, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            Set(line.substr(0, eq), line.substr(eq + 1));
        }
        return true;
    }

    bool SaveFile(const std::string& path) const {
        std::ofstream out(path);
        if (!out) return false;
        for (auto& [k, v] : m_Map) out << k << "=" << v << "\n";
        return true;
    }

private:
    std::unordered_map<std::string, std::string> m_Map;
};

} // namespace Muk
