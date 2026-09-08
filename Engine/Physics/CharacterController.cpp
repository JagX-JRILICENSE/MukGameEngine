#include "CharacterController.h"
#include "PhysicsWorld.h"
#include "Core/Log.h"
#include <cmath>

#ifdef MUK_USE_JOLT
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#endif

namespace Muk {

struct CharacterController::Impl {
#ifdef MUK_USE_JOLT
    JPH::CharacterVirtual* Character = nullptr;
    JPH::PhysicsSystem* System = nullptr;
    JPH::TempAllocatorImpl* Temp = nullptr;
#endif
};

CharacterController::CharacterController() : m_Impl(std::make_unique<Impl>()) {}
CharacterController::~CharacterController() = default;

bool CharacterController::Create(PhysicsWorld& world, const CharacterDesc& desc) {
    Destroy(world);
    m_Desc = desc;
    m_Position = desc.Position;
    m_Velocity = {};

#ifdef MUK_USE_JOLT
    if (world.IsUsingJolt()) {
        auto* sys = static_cast<JPH::PhysicsSystem*>(world.GetJoltSystemPtr());
        auto* temp = static_cast<JPH::TempAllocatorImpl*>(world.GetJoltTempAllocatorPtr());
        if (!sys || !temp) {
            MUK_CORE_ERROR("CharacterController: no Jolt system");
            return false;
        }

        JPH::RefConst<JPH::Shape> standingShape = JPH::RotatedTranslatedShapeSettings(
            JPH::Vec3(0, 0.5f * desc.Height + desc.Radius, 0),
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShape(0.5f * desc.Height, desc.Radius)
        ).Create().Get();

        JPH::CharacterVirtualSettings settings;
        settings.mMass = 70.0f;
        settings.mMaxSlopeAngle = desc.MaxSlopeDegrees * (3.14159265f / 180.0f);
        settings.mMaxStrength = desc.Strength;
        settings.mShape = standingShape;
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -desc.Radius);

        m_Impl->Character = new JPH::CharacterVirtual(&settings,
            JPH::RVec3(desc.Position.x, desc.Position.y, desc.Position.z),
            JPH::Quat::sIdentity(), 0, sys);
        m_Impl->System = sys;
        m_Impl->Temp = temp;
        m_Valid = true;
        MUK_CORE_INFO("CharacterController: Jolt CharacterVirtual ready");
        return true;
    }
#else
    (void)world;
#endif

    m_Valid = true;
    MUK_CORE_INFO("CharacterController: simple kinematic mode");
    return true;
}

void CharacterController::Destroy(PhysicsWorld&) {
#ifdef MUK_USE_JOLT
    if (m_Impl->Character) {
        delete m_Impl->Character;
        m_Impl->Character = nullptr;
    }
    m_Impl->System = nullptr;
    m_Impl->Temp = nullptr;
#endif
    m_Valid = false;
}

void CharacterController::SetMoveInput(const Vec3& wishDir, f32 speed) {
    f32 len = std::sqrt(wishDir.x * wishDir.x + wishDir.z * wishDir.z);
    if (len > 1e-4f)
        m_WishDir = { wishDir.x / len, 0, wishDir.z / len };
    else
        m_WishDir = { 0, 0, 0 };
    m_Speed = speed;
}

void CharacterController::Jump(f32 impulse) {
    if (m_Grounded)
        m_JumpImpulse = impulse;
}

void CharacterController::Update(PhysicsWorld& world, f32 dt) {
    if (!m_Valid) return;

#ifdef MUK_USE_JOLT
    if (m_Impl->Character && m_Impl->System) {
        JPH::Vec3 wish = JPH::Vec3(m_WishDir.x, 0, m_WishDir.z) * m_Speed;

        JPH::CharacterVirtual::ExtendedUpdateSettings upd;
        upd.mStickToFloorStepDown = JPH::Vec3(0, -0.5f, 0);
        upd.mWalkStairsStepUp = JPH::Vec3(0, 0.4f, 0);

        JPH::Vec3 velocity = m_Impl->Character->GetLinearVelocity();
        velocity.SetX(wish.GetX());
        velocity.SetZ(wish.GetZ());

        if (m_JumpImpulse > 0.0f) {
            velocity.SetY(m_JumpImpulse);
            m_JumpImpulse = 0.0f;
        }

        Vec3 g = world.GetGravity();
        if (!m_Impl->Character->IsSupported())
            velocity += JPH::Vec3(g.x, g.y, g.z) * dt;

        m_Impl->Character->SetLinearVelocity(velocity);

        m_Impl->Character->ExtendedUpdate(
            dt,
            m_Impl->System->GetGravity(),
            upd,
            m_Impl->System->GetDefaultBroadPhaseLayerFilter(1),
            m_Impl->System->GetDefaultLayerFilter(1),
            {},
            {},
            *m_Impl->Temp
        );

        JPH::RVec3 p = m_Impl->Character->GetPosition();
        m_Position = { (f32)p.GetX(), (f32)p.GetY(), (f32)p.GetZ() };
        JPH::Vec3 v = m_Impl->Character->GetLinearVelocity();
        m_Velocity = { v.GetX(), v.GetY(), v.GetZ() };
        m_Grounded = m_Impl->Character->IsSupported();
        return;
    }
#endif

    m_Velocity.x = m_WishDir.x * m_Speed;
    m_Velocity.z = m_WishDir.z * m_Speed;
    Vec3 g = world.GetGravity();
    if (!m_Grounded)
        m_Velocity.y += g.y * dt;

    if (m_JumpImpulse > 0.0f) {
        m_Velocity.y = m_JumpImpulse;
        m_JumpImpulse = 0.0f;
        m_Grounded = false;
    }

    m_Position = m_Position + m_Velocity * dt;

    f32 feet = m_Desc.Radius;
    if (m_Position.y < feet) {
        m_Position.y = feet;
        m_Velocity.y = 0;
        m_Grounded = true;
    } else if (m_Position.y > feet + 0.05f) {
        m_Grounded = false;
    }
}

Vec3 CharacterController::GetPosition() const { return m_Position; }
Vec3 CharacterController::GetVelocity() const { return m_Velocity; }

} // namespace Muk
