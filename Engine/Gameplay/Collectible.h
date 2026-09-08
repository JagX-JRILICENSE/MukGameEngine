#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include <string>
#include <functional>
#include <vector>

namespace Muk {

class ParticleSystem;
class AudioSystem;

struct CollectibleComponent : public IComponent {
    float Radius = 1.25f;
    int ScoreValue = 1;
    bool Collected = false;
    std::string Tag = "orb"; // for filtering
};

struct CollectorComponent : public IComponent {
    float Radius = 0.6f;
    int Score = 0;
    int TargetScore = 3;
};

/** Distance-based collectible pickup; emits particles + score callback */
class CollectibleSystem {
public:
    using OnCollectFn = std::function<void(Entity orb, int scoreValue, int totalScore)>;
    using OnCompleteFn = std::function<void(int totalScore)>;

    void SetOnCollect(OnCollectFn fn) { m_OnCollect = std::move(fn); }
    void SetOnComplete(OnCompleteFn fn) { m_OnComplete = std::move(fn); }

    void Update(World& world, ParticleSystem* particles, AudioSystem* audio);

    static Entity SpawnOrb(World& world, const Vec3& pos, int scoreValue = 1,
                           std::vector<struct EditorEntityInfo>* track = nullptr);

private:
    OnCollectFn m_OnCollect;
    OnCompleteFn m_OnComplete;
};

} // namespace Muk
