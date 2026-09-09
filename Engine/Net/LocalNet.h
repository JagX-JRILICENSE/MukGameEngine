#pragma once
#include "Math/Vector.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Muk {

struct NetPlayer {
    uint32_t Id = 0;
    std::string Name;
    Vec3 Position{};
    bool Local = false;
};

/** Local-authority multiplayer scaffold (LAN ready architecture) */
class LocalNet {
public:
    void Host(const std::string& name = "Host") {
        m_Hosting = true;
        m_Players.clear();
        NetPlayer p; p.Id = 1; p.Name = name; p.Local = true;
        m_Players.push_back(p);
        m_LocalId = 1;
    }

    void JoinSimulated(const std::string& name) {
        NetPlayer p; p.Id = (uint32_t)m_Players.size() + 1; p.Name = name; p.Local = false;
        m_Players.push_back(p);
    }

    void UpdateLocalPos(const Vec3& pos) {
        for (auto& p : m_Players) if (p.Id == m_LocalId) p.Position = pos;
    }

    void Tick(float /*dt*/) {
        // Placeholder for state replication
        ++m_Tick;
    }

    bool IsHosting() const { return m_Hosting; }
    uint32_t TickIndex() const { return m_Tick; }
    const std::vector<NetPlayer>& Players() const { return m_Players; }

private:
    bool m_Hosting = false;
    uint32_t m_LocalId = 0;
    uint32_t m_Tick = 0;
    std::vector<NetPlayer> m_Players;
};

} // namespace Muk
