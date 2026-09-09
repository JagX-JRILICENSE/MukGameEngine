#include "TriggerVolume.h"
#include "Core/Log.h"
#include <cmath>

namespace Muk {

Entity TriggerSystem::SpawnBox(World& world, const Vec3& pos, const Vec3& halfExtents,
                               const std::string& tag, const std::string& name) {
    auto e = world.CreateEntity();
    world.AddComponent<NameComponent>(e).Name = name;
    auto& t = world.AddComponent<Transform>(e);
    t.Position = pos;
    t.Scale = { halfExtents.x * 2, halfExtents.y * 2, halfExtents.z * 2 };
    auto& tv = world.AddComponent<TriggerVolume>(e);
    tv.HalfExtents = halfExtents;
    tv.Tag = tag;
    // Invisible by default — no MeshRenderer
    return e;
}

void TriggerSystem::Update(World& world) {
    struct T { Entity E; Transform* Tr; TriggerVolume* V; };
    struct O { Entity E; Transform* Tr; Vec3 Half; };
    std::vector<T> triggers;
    std::vector<O> others;

    world.ForEach<TriggerVolume, Transform>([&](Entity e, TriggerVolume& v, Transform& t) {
        triggers.push_back({ e, &t, &v });
    });
    world.ForEach<Transform, MeshRenderer>([&](Entity e, Transform& t, MeshRenderer& mr) {
        if (!mr.Visible) return;
        // Skip pure triggers if they also have mesh
        if (world.GetComponent<TriggerVolume>(e)) return;
        Vec3 half{ t.Scale.x * 0.5f, t.Scale.y * 0.5f, t.Scale.z * 0.5f };
        others.push_back({ e, &t, half });
    });
    // Also collectors/players without requiring MeshRenderer scale
    world.ForEach<CollectorComponent, Transform>([&](Entity e, CollectorComponent&, Transform& t) {
        others.push_back({ e, &t, { 0.4f, 0.9f, 0.4f } });
    });

    std::unordered_set<u64> nowActive;
    for (auto& tr : triggers) {
        if (tr.V->Once && tr.V->Fired) continue;
        for (auto& o : others) {
            if (o.E.GetID() == tr.E.GetID()) continue;
            bool hit = AABBOverlap(tr.Tr->Position, tr.V->HalfExtents, o.Tr->Position, o.Half);
            u64 key = (u64)tr.E.GetID() << 32 | (u64)o.E.GetID();
            if (hit) {
                nowActive.insert(key);
                if (!m_Active.count(key)) {
                    tr.V->Overlapping = true;
                    if (m_OnEnter) m_OnEnter(tr.E, o.E, tr.V->Tag);
                    if (tr.V->Once) tr.V->Fired = true;
                    MUK_CORE_INFO("Trigger enter tag={0}", tr.V->Tag.c_str());
                }
            }
        }
    }
    for (auto key : m_Active) {
        if (!nowActive.count(key) && m_OnExit) {
            EntityID tid = (EntityID)(key >> 32);
            EntityID oid = (EntityID)(key & 0xffffffffu);
            m_OnExit(Entity{ tid }, Entity{ oid }, "");
        }
    }
    m_Active.swap(nowActive);
}

} // namespace Muk
