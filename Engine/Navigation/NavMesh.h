#pragma once
#include "GridPath.h"
#include "Math/Vector.h"
#include <vector>

namespace Muk {

/** Bake a coarse navmesh from GridPathfinder walkability (UE-like nav for AI) */
class NavMesh {
public:
    void Bake(const GridPathfinder& grid) {
        m_W = grid.Width(); m_D = grid.Depth(); m_Cell = grid.CellSize();
        m_Tris.clear();
        for (int z = 0; z < m_D - 1; ++z) {
            for (int x = 0; x < m_W - 1; ++x) {
                if (!grid.IsWalkable(x, z) || !grid.IsWalkable(x+1, z) ||
                    !grid.IsWalkable(x, z+1) || !grid.IsWalkable(x+1, z+1))
                    continue;
                Vec3 a = grid.CellToWorld(x, z);
                Vec3 b = grid.CellToWorld(x+1, z);
                Vec3 c = grid.CellToWorld(x, z+1);
                Vec3 d = grid.CellToWorld(x+1, z+1);
                m_Tris.push_back({ a, b, d });
                m_Tris.push_back({ a, d, c });
            }
        }
    }

    int TriangleCount() const { return (int)m_Tris.size(); }

    struct Tri { Vec3 A, B, C; };
    const std::vector<Tri>& Tris() const { return m_Tris; }

    std::vector<Vec3> FindPath(const GridPathfinder& grid, const Vec3& start, const Vec3& goal) const {
        return grid.FindPath(start, goal);
    }

private:
    int m_W = 0, m_D = 0;
    float m_Cell = 1.f;
    std::vector<Tri> m_Tris;
};

} // namespace Muk
