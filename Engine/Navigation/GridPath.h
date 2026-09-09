#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace Muk {

struct GridCell {
    int X = 0, Z = 0;
    bool Walkable = true;
};

/**
 * Simple 2D grid pathfinding (X/Z plane).
 * World positions map via cell size and origin.
 */
class GridPathfinder {
public:
    void Configure(int width, int depth, float cellSize, const Vec3& origin) {
        m_W = width; m_D = depth; m_Cell = cellSize; m_Origin = origin;
        m_Walk.assign(width * depth, true);
    }

    void SetBlocked(int x, int z, bool blocked) {
        if (!InBounds(x, z)) return;
        m_Walk[z * m_W + x] = !blocked;
    }

    void BlockWorldCircle(const Vec3& pos, float radius) {
        int r = (int)std::ceil(radius / m_Cell);
        int cx, cz; WorldToCell(pos, cx, cz);
        for (int z = cz - r; z <= cz + r; ++z)
            for (int x = cx - r; x <= cx + r; ++x)
                if (InBounds(x, z)) {
                    float dx = (float)(x - cx), dz = (float)(z - cz);
                    if (dx * dx + dz * dz <= (float)(r * r))
                        SetBlocked(x, z, true);
                }
    }

    void ClearBlocks() { std::fill(m_Walk.begin(), m_Walk.end(), true); }

    bool WorldToCell(const Vec3& p, int& x, int& z) const {
        x = (int)std::floor((p.x - m_Origin.x) / m_Cell);
        z = (int)std::floor((p.z - m_Origin.z) / m_Cell);
        return InBounds(x, z);
    }

    Vec3 CellToWorld(int x, int z) const {
        return { m_Origin.x + (x + 0.5f) * m_Cell, m_Origin.y,
                 m_Origin.z + (z + 0.5f) * m_Cell };
    }

    bool InBounds(int x, int z) const {
        return x >= 0 && z >= 0 && x < m_W && z < m_D;
    }

    bool IsWalkable(int x, int z) const {
        return InBounds(x, z) && m_Walk[z * m_W + x];
    }

    /** A* returns world waypoints (empty if none) */
    std::vector<Vec3> FindPath(const Vec3& start, const Vec3& goal) const {
        int sx, sz, gx, gz;
        if (!WorldToCell(start, sx, sz) || !WorldToCell(goal, gx, gz))
            return {};
        if (!IsWalkable(sx, sz) || !IsWalkable(gx, gz))
            return {};

        auto key = [&](int x, int z) { return z * m_W + x; };
        auto heur = [&](int x, int z) {
            return (float)(std::abs(x - gx) + std::abs(z - gz));
        };

        struct Node { int X, Z; float F; };
        struct Cmp { bool operator()(const Node& a, const Node& b) const { return a.F > b.F; } };
        std::priority_queue<Node, std::vector<Node>, Cmp> open;
        std::vector<float> gScore(m_W * m_D, 1e30f);
        std::vector<int> parent(m_W * m_D, -1);
        std::vector<char> closed(m_W * m_D, 0);

        int sKey = key(sx, sz);
        gScore[sKey] = 0;
        open.push({ sx, sz, heur(sx, sz) });

        const int dirs[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };

        while (!open.empty()) {
            Node cur = open.top(); open.pop();
            int ck = key(cur.X, cur.Z);
            if (closed[ck]) continue;
            closed[ck] = 1;
            if (cur.X == gx && cur.Z == gz) break;

            for (auto& d : dirs) {
                int nx = cur.X + d[0], nz = cur.Z + d[1];
                if (!IsWalkable(nx, nz)) continue;
                int nk = key(nx, nz);
                if (closed[nk]) continue;
                float step = (d[0] != 0 && d[1] != 0) ? 1.414f : 1.0f;
                float ng = gScore[ck] + step;
                if (ng < gScore[nk]) {
                    gScore[nk] = ng;
                    parent[nk] = ck;
                    open.push({ nx, nz, ng + heur(nx, nz) });
                }
            }
        }

        int gk = key(gx, gz);
        if (parent[gk] < 0 && !(sx == gx && sz == gz)) return {};

        std::vector<Vec3> path;
        int k = gk;
        while (k >= 0) {
            int x = k % m_W, z = k / m_W;
            path.push_back(CellToWorld(x, z));
            if (k == sKey) break;
            k = parent[k];
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    /** Move entity along path; returns new position */
    static Vec3 FollowPath(std::vector<Vec3>& path, const Vec3& current, float speed, float dt) {
        if (path.empty()) return current;
        Vec3 target = path.front();
        float dx = target.x - current.x, dz = target.z - current.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        float step = speed * dt;
        if (dist <= step + 0.05f) {
            path.erase(path.begin());
            return { target.x, current.y, target.z };
        }
        float inv = 1.0f / dist;
        return { current.x + dx * inv * step, current.y, current.z + dz * inv * step };
    }

    int Width() const { return m_W; }
    int Depth() const { return m_D; }
    float CellSize() const { return m_Cell; }

private:
    int m_W = 0, m_D = 0;
    float m_Cell = 1.0f;
    Vec3 m_Origin{ -10, 0, -10 };
    std::vector<char> m_Walk;
};

} // namespace Muk
