#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Muk {

struct PluginInfo {
    std::string Name;
    std::string Version;
    std::string Author;
    std::string Description;
    bool Enabled = true;
};

/** Lightweight plugin/module registry — ecosystem foundation */
class PluginRegistry {
public:
    using InitFn = std::function<void()>;
    using TickFn = std::function<void(float)>;

    void Register(const PluginInfo& info, InitFn init = {}, TickFn tick = {}) {
        m_Plugins[info.Name] = info;
        if (init) m_Inits[info.Name] = std::move(init);
        if (tick) m_Ticks[info.Name] = std::move(tick);
    }

    void Enable(const std::string& name, bool on) {
        if (m_Plugins.count(name)) m_Plugins[name].Enabled = on;
    }

    void InitializeAll() {
        for (auto& [n, p] : m_Plugins)
            if (p.Enabled && m_Inits.count(n)) m_Inits[n]();
    }

    void TickAll(float dt) {
        for (auto& [n, p] : m_Plugins)
            if (p.Enabled && m_Ticks.count(n)) m_Ticks[n](dt);
    }

    const std::unordered_map<std::string, PluginInfo>& All() const { return m_Plugins; }

    void RegisterBuiltinEcosystem() {
        Register({ "Muk.AI", "0.14", "Muk", "AI game builder agents", true });
        Register({ "Muk.Net", "0.14", "Muk", "UDP multiplayer", true });
        Register({ "Muk.PostFX", "0.14", "Muk", "SSAO + Bloom", true });
        Register({ "Muk.Anim", "0.14", "Muk", "Montages + blend trees", true });
        Register({ "Muk.Nodes", "0.14", "Muk", "Visual scripting graph", true });
        Register({ "Muk.Terrain", "0.14", "Muk", "Heightfield + water", true });
        Register({ "Muk.Foliage", "0.14", "Muk", "Vegetation scatter", true });
        Register({ "Muk.Cinema", "0.14", "Muk", "Timeline sequencer", true });
        Register({ "Sample.OrbGame", "0.14", "Muk", "Collectible sample content", true });
        Register({ "Sample.Arena", "0.14", "Muk", "Arena layout prefabs", true });
    }

private:
    std::unordered_map<std::string, PluginInfo> m_Plugins;
    std::unordered_map<std::string, InitFn> m_Inits;
    std::unordered_map<std::string, TickFn> m_Ticks;
};

} // namespace Muk
