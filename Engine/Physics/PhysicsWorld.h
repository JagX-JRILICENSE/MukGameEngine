#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "ECS/Entity.h"
#include <vector>
#include <memory>
#include <unordered_map>

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
    Mesh
};

struct RigidBodyDesc {
    BodyType Type = BodyType::Dynamic;
    ShapeType Shape = ShapeType::Box;
    Vec3 Position{0, 0, 0};
    Vec3 Rotation{0, 0, 0}; // Euler degrees
    Vec3 HalfExtents{0.5f, 0.5f, 0.5f};
    f32 Radius = 0.5f;
    f32 Height = 1.0f;
    f32 Mass = 1.0f;
    f32 Friction = 0.5f;
    f32 Restitution = 0.0f;
    bool GravityEnabled = true;
};

/**
 * Physics abstraction.
 * - Default: simple CPU solver (always available)
 * - When built with MUK_USE_JOLT: uses Jolt Physics
 */
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void Initialize();
    void Shutdown();

    EntityID CreateBody(const RigidBodyDesc& desc);
    void DestroyBody(EntityID body);

    void SetGravity(const Vec3& gravity);
    Vec3 GetGravity() const;

    void Update(f32 deltaTime);

    Vec3 GetBodyPosition(EntityID body) const;
    void SetBodyPosition(EntityID body, const Vec3& pos);
    Vec3 GetBodyVelocity(EntityID body) const;

    bool IsUsingJolt() const;

private:
    struct SimpleBody {
        EntityID Id = 0;
        RigidBodyDesc Desc;
        Vec3 Position;
        Vec3 Velocity;
        bool Active = true;
    };

    // Simple backend data
    std::vector<SimpleBody> m_SimpleBodies;
    Vec3 m_Gravity{0.0f, -9.81f, 0.0f};
    bool m_Initialized = false;
    EntityID m_NextBodyId = 1;

    // Jolt backend (opaque to avoid heavy includes in header)
    struct JoltState;
    std::unique_ptr<JoltState> m_Jolt;

    void InitSimple();
    void InitJolt();
    void UpdateSimple(f32 dt);
    void UpdateJolt(f32 dt);
    void ShutdownJolt();
};

} // namespace Muk
