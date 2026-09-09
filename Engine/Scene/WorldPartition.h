#pragma once
#include "Math/Vector.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>

namespace Muk {

struct WorldChunk {
    int X = 0, Z = 0;
    bool Loaded = false;
    std::string SceneFile;
};

class WorldPartition {
public:
    void Configure(float chunkSize = 32.f, int loadRadius = 1) {
        m_Size = chunkSize; m_Radius = loadRadius;
    }

    void RegisterChunk(int cx, int cz, const std::string& file) {
        m_Chunks[Key(cx, cz)] = { cx, cz, false, file };
    }

    void UpdateStreaming(const Vec3& playerPos, std::vector<std::string>& toLoad,
                         std::vector<std::string>& toUnload) {
        int px = (int)std::floor(playerPos.x / m_Size);
        int pz = (int)std::floor(playerPos.z / m_Size);
        for (auto& [k, c] : m_Chunks) {
            int dx = std::abs(c.X - px), dz = std::abs(c.Z - pz);
            bool should = (dx <= m_Radius && dz <= m_Radius);
            if (should && !c.Loaded) { c.Loaded = true; toLoad.push_back(c.SceneFile); }
            if (!should && c.Loaded) { c.Loaded = false; toUnload.push_back(c.SceneFile); }
        }
    }

    int ChunkCount() const { return (int)m_Chunks.size(); }
    int LoadedCount() const {
        int n = 0; for (auto& [k, c] : m_Chunks) if (c.Loaded) ++n; return n;
    }

private:
    static long long Key(int x, int z) { return ((long long)x << 32) ^ (unsigned)z; }
    float m_Size = 32.f;
    int m_Radius = 1;
    std::unordered_map<long long, WorldChunk> m_Chunks;
};

} // namespace Muk
