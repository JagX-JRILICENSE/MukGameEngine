#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <memory>

namespace Muk {

class PhysicsWorld;

struct CharacterDesc {
    Vec3 Position{0, 1, 0};
    f32 Radius = 0.3f;
    f32 Height = 1.2f; // capsule cylinder height (total height ~= height + 2*radius)
    f32 MaxSlopeDegrees = 45.0f;
    f32 Strength = 100.0f;
};

/**
 * First-person / third-person style character.
 * Uses Jolt CharacterVirtual when MUK_USE_JOLT, else a simple kinematic capsule.
 */
class CharacterController {
public:
    CharacterController();
    ~CharacterController();

    bool Create(PhysicsWorld& world, const CharacterDesc& desc);
    void Destroy(PhysicsWorld& world);

    void SetMoveInput(const Vec3& wishDir, f32 speed);
    void Jump(f32 impulse = 6.0f);
    void Update(PhysicsWorld& world, f32 dt);

    Vec3 GetPosition() const;
    Vec3 GetVelocity() const;
    bool IsGrounded() const { return m_Grounded; }
    bool IsValid() const { return m_Valid; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
    bool m_Valid = false;
    bool m_Grounded = false;
    Vec3 m_Position{0, 1, 0};
    Vec3 m_Velocity{0, 0, 0};
    Vec3 m_WishDir{0, 0, 0};
    f32 m_Speed = 5.0f;
    f32 m_JumpImpulse = 0.0f;
    CharacterDesc m_Desc;
};

} // namespace Muk
