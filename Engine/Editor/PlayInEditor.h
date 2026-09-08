#pragma once

#include "Core/Core.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include "Math/Vector.h"
#include <vector>
#include <unordered_map>

namespace Muk {

class World;
class CharacterController;

struct PIETransformSnapshot {
    EntityID Id = 0;
    Transform T;
};

/**
 * Play-In-Editor: snapshot transforms on Play, restore on Stop.
 * While playing, game systems (character, physics) run; gizmos disabled.
 */
class PlayInEditor {
public:
    void Play(World& world, CharacterController* character = nullptr);
    void Stop(World& world, CharacterController* character = nullptr);
    void Toggle(World& world, CharacterController* character = nullptr);

    bool IsPlaying() const { return m_Playing; }

private:
    bool m_Playing = false;
    std::vector<PIETransformSnapshot> m_Snapshots;
    Vec3 m_CharPos{};
    bool m_HadChar = false;
};

} // namespace Muk
