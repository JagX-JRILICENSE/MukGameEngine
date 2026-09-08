#include "PhysicsWorld.h"
#include "Core/Log.h"

namespace Muk {

PhysicsWorld::PhysicsWorld() = default;
PhysicsWorld::~PhysicsWorld() { Shutdown(); }

void PhysicsWorld::Initialize() {
    m_Initialized = true;
    MUK_CORE_INFO("PhysicsWorld initialized (simple CPU solver - replace with Jolt later)");
}

void PhysicsWorld::Shutdown() {
    m_Bodies.clear();
    m_Initialized = false;
}

EntityID PhysicsWorld::CreateBody(const RigidBodyDesc& desc) {
    Body body;
    body.Id = m_NextBodyId++;
    body.Desc = desc;
    body.Position = desc.Position;
    body.Velocity = {0, 0, 0};
    body.AngularVelocity = {0, 0, 0};
    body.Active = true;

    m_Bodies.push_back(body);
    MUK_CORE_TRACE("Created physics body {0}", body.Id);
    return body.Id;
}

void PhysicsWorld::DestroyBody(EntityID body) {
    for (auto it = m_Bodies.begin(); it != m_Bodies.end(); ++it) {
        if (it->Id == body) {
            m_Bodies.erase(it);
            return;
        }
    }
}

void PhysicsWorld::SetGravity(const Vec3& gravity) {
    m_Gravity = gravity;
}

void PhysicsWorld::Update(f32 deltaTime) {
    if (!m_Initialized) return;

    // Extremely simple Euler integration + ground plane at y=0
    for (auto& body : m_Bodies) {
        if (!body.Active || body.Desc.Type == BodyType::Static)
            continue;

        if (body.Desc.GravityEnabled) {
            body.Velocity = body.Velocity + m_Gravity * deltaTime;
        }

        body.Position = body.Position + body.Velocity * deltaTime;

        // Simple ground collision
        f32 groundY = 0.0f;
        if (body.Desc.Shape == ShapeType::Box) {
            groundY = body.Desc.HalfExtents.y;
        } else if (body.Desc.Shape == ShapeType::Sphere) {
            groundY = body.Desc.Radius;
        }

        if (body.Position.y < groundY) {
            body.Position.y = groundY;
            body.Velocity.y = -body.Velocity.y * body.Desc.Restitution;
            // Apply friction
            body.Velocity.x *= (1.0f - body.Desc.Friction * deltaTime * 5.0f);
            body.Velocity.z *= (1.0f - body.Desc.Friction * deltaTime * 5.0f);
        }
    }
}

Vec3 PhysicsWorld::GetBodyPosition(EntityID body) const {
    for (const auto& b : m_Bodies) {
        if (b.Id == body) return b.Position;
    }
    return {};
}

void PhysicsWorld::SetBodyPosition(EntityID body, const Vec3& pos) {
    for (auto& b : m_Bodies) {
        if (b.Id == body) {
            b.Position = pos;
            return;
        }
    }
}

} // namespace Muk
