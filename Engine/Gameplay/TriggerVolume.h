#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include <string>
#include <functional>
#include <unordered_set>

namespace Muk {

struct TriggerVolume : public IComponent {
    Vec3 HalfExtents{1, 1, 1};
    std::string Tag = "trigger";
    bool Once = false;
    bool Fired = false;
    bool Overlapping = false;
};

/** AABB overlap triggers — fires enter/exit callbacks */
class TriggerSystem {
public:
    using Fn = std::function<void(Entity trigger, Entity other, const std::string& tag)>;

    void SetOnEnter(Fn fn) { m_OnEnter = std::move(fn); }
    void SetOnExit(Fn fn) { m_OnExit = std::move(fn); }

    void Update(World& world);

    static Entity SpawnBox(World& world, const Vec3& pos, const Vec3& halfExtents,
                           const std::string& tag, const std::string& name = "Trigger");

private:
    Fn m_OnEnter, m_OnExit;
    std::unordered_set<u64> m_Active; // (triggerId<<32)|otherId

    static bool AABBOverlap(const Vec3& aPos, const Vec3& aHalf,
                            const Vec3& bPos, const Vec3& bHalf) {
        return std::fabs(aPos.x - bPos.x) <= (aHalf.x + bHalf.x)
            && std::fabs(aPos.y - bPos.y) <= (aHalf.y + bHalf.y)
            && std::fabs(aPos.z - bPos.z) <= (aHalf.z + bHalf.z);
    }
};

} // namespace Muk
