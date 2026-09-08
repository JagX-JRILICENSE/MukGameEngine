#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "ECS/Entity.h"
#include <vector>
#include <memory>

namespace Muk {

enum class BodyType {
    Static,
    Dynamic,
    Kinematic
};

enum class ShapeType {
    Box,
    Sphere,
    Capsule,
    Mesh // later
};

struct RigidBodyDesc {
    BodyType Type = BodyType::Dynamic;
    ShapeType Shape = ShapeType::Box;
    Vec3 Position{0, 0, 0};
    Vec3 Rotation{0, 0, 0}; // Euler degrees for simplicity
    Vec3 HalfExtents{0.5f, 0.5f, 0.5f}; // for box
    f32 Radius = 0.5f;                   // for sphere/capsule
    f32 Height = 1.0f;                   // for capsule
    f32 Mass = 1.0f;
    f32 Friction = 0.5f;
    f32 Restitution = 0.0f;
    bool GravityEnabled = true;
};

/**
 * Physics abstraction.
 * Currently a simple CPU simulation placeholder.
 * Designed so Jolt Physics can be dropped in later with minimal API changes.
 */
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void Initialize();
    void Shutdown();

    // Create a body and return a handle (EntityID for now)
    EntityID CreateBody(const RigidBodyDesc& desc);
    void DestroyBody(EntityID body);

    void SetGravity(const Vec3& gravity);
    Vec3 GetGravity() const { return m_Gravity; }

    // Step the simulation
    void Update(f32 deltaTime);

    // Query
    Vec3 GetBodyPosition(EntityID body) const;
    void SetBodyPosition(EntityID body, const Vec3& pos);

    // Future: Raycast, Overlap, Constraints, Vehicles, Character controllers...

private:
    struct Body {
        EntityID Id = 0;
        RigidBodyDesc Desc;
        Vec3 Position;
        Vec3 Velocity;
        Vec3 AngularVelocity;
        bool Active = true;
    };

    std::vector<Body> m_Bodies;
    Vec3 m_Gravity{0.0f, -9.81f, 0.0f};
    bool m_Initialized = false;
    EntityID m_NextBodyId = 1;
};

} // namespace Muk
