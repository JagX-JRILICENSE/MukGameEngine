#pragma once
#include "ECS/Entity.h"
#include "Math/Vector.h"
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace Muk {

class MultiSelect {
public:
    void Clear() { m_Ids.clear(); }
    void Set(Entity e) { m_Ids.clear(); if (e.IsValid()) m_Ids.insert(e.GetID()); }
    void Toggle(Entity e) {
        if (!e.IsValid()) return;
        auto id = e.GetID();
        if (m_Ids.count(id)) m_Ids.erase(id); else m_Ids.insert(id);
    }
    void Add(Entity e) { if (e.IsValid()) m_Ids.insert(e.GetID()); }
    bool Contains(Entity e) const { return e.IsValid() && m_Ids.count(e.GetID()) > 0; }
    size_t Count() const { return m_Ids.size(); }
    std::vector<Entity> Entities() const {
        std::vector<Entity> out;
        for (auto id : m_Ids) out.emplace_back(id);
        return out;
    }
    Entity Primary() const {
        if (m_Ids.empty()) return {};
        return Entity{ *m_Ids.begin() };
    }

    /** Screen-space AABB box select (normalized 0-1 viewport) */
    void BeginBox(float x, float y) { m_Box0 = { x, y }; m_Boxing = true; }
    void UpdateBox(float x, float y) { m_Box1 = { x, y }; }
    bool IsBoxing() const { return m_Boxing; }
    void EndBox() { m_Boxing = false; }
    void GetBox(float& minX, float& minY, float& maxX, float& maxY) const {
        minX = std::min(m_Box0.x, m_Box1.x); maxX = std::max(m_Box0.x, m_Box1.x);
        minY = std::min(m_Box0.y, m_Box1.y); maxY = std::max(m_Box0.y, m_Box1.y);
    }

private:
    std::unordered_set<EntityID> m_Ids;
    bool m_Boxing = false;
    Vec3 m_Box0{}, m_Box1{};
};

} // namespace Muk
