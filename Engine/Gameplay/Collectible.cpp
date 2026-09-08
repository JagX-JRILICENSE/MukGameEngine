#include "Collectible.h"
#include "EditorUI/EditorUI.h"
#include "Particles/ParticleSystem.h"
#include "Audio/AudioSystem.h"
#include "Core/Log.h"
#include <cmath>

namespace Muk {

Entity CollectibleSystem::SpawnOrb(World& world, const Vec3& pos, int scoreValue,
                                   std::vector<EditorEntityInfo>* track) {
    auto e = world.CreateEntity();
    static int s_Orb = 1;
    std::string name = "Orb" + std::to_string(s_Orb++);
    world.AddComponent<NameComponent>(e).Name = name;
    auto& t = world.AddComponent<Transform>(e);
    t.Position = pos;
    t.Scale = { 0.35f, 0.35f, 0.35f };
    auto& mr = world.AddComponent<MeshRenderer>(e);
    mr.MeshName = "Cube";
    mr.MaterialName = "Default";
    auto& c = world.AddComponent<CollectibleComponent>(e);
    c.Radius = 1.25f;
    c.ScoreValue = scoreValue;
    c.Tag = "orb";
    if (track) track->push_back({ e, name, false });
    return e;
}

void CollectibleSystem::Update(World& world, ParticleSystem* particles, AudioSystem* audio) {
    // Find collectors
    struct Coll {
        Entity E;
        Transform* T;
        CollectorComponent* C;
    };
    std::vector<Coll> collectors;
    world.ForEach<CollectorComponent, Transform>([&](Entity e, CollectorComponent& c, Transform& t) {
        collectors.push_back({ e, &t, &c });
    });
    if (collectors.empty()) return;

    std::vector<Entity> toHide;
    world.ForEach<CollectibleComponent, Transform, MeshRenderer>(
        [&](Entity e, CollectibleComponent& col, Transform& ot, MeshRenderer& mr) {
            if (col.Collected) return;
            for (auto& coll : collectors) {
                float dx = coll.T->Position.x - ot.Position.x;
                float dy = coll.T->Position.y - ot.Position.y;
                float dz = coll.T->Position.z - ot.Position.z;
                float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                float reach = col.Radius + coll.C->Radius;
                if (dist <= reach) {
                    col.Collected = true;
                    mr.Visible = false;
                    coll.C->Score += col.ScoreValue;
                    if (particles) particles->Burst(ot.Position, 24, { 1.0f, 0.85f, 0.2f });
                    if (audio) {
                        AudioSourceDesc d;
                        d.ClipName = "success";
                        d.Spatial = true;
                        d.Position = ot.Position;
                        audio->Play(d);
                    }
                    if (m_OnCollect) m_OnCollect(e, col.ScoreValue, coll.C->Score);
                    if (coll.C->Score >= coll.C->TargetScore && m_OnComplete)
                        m_OnComplete(coll.C->Score);
                    MUK_CORE_INFO("Collected {0} score={1}", col.Tag.c_str(), coll.C->Score);
                    break;
                }
            }
        });
}

} // namespace Muk
