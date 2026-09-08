#include "PhysicsWorld.h"
#include "Core/Log.h"

#include <thread>

#ifdef MUK_USE_JOLT
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace {
    constexpr JPH::ObjectLayer LAYER_NON_MOVING = 0;
    constexpr JPH::ObjectLayer LAYER_MOVING = 1;
    constexpr JPH::BroadPhaseLayer BP_NON_MOVING(0);
    constexpr JPH::BroadPhaseLayer BP_MOVING(1);

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        BPLayerInterfaceImpl() {
            mObjectToBroadPhase[LAYER_NON_MOVING] = BP_NON_MOVING;
            mObjectToBroadPhase[LAYER_MOVING] = BP_MOVING;
        }
        virtual unsigned GetNumBroadPhaseLayers() const override { return 2; }
        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
            return mObjectToBroadPhase[inLayer];
        }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
            return (unsigned)inLayer == 0 ? "NON_MOVING" : "MOVING";
        }
#endif
    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[2];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
            switch (inLayer1) {
                case LAYER_NON_MOVING: return inLayer2 == BP_MOVING;
                case LAYER_MOVING: return true;
                default: return false;
            }
        }
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
            switch (inObject1) {
                case LAYER_NON_MOVING: return inObject2 == LAYER_MOVING;
                case LAYER_MOVING: return true;
                default: return false;
            }
        }
    };

    static BPLayerInterfaceImpl s_BPLayers;
    static ObjectVsBroadPhaseLayerFilterImpl s_ObjectVsBroadPhase;
    static ObjectLayerPairFilterImpl s_ObjectVsObject;
}
#endif

namespace Muk {

#ifdef MUK_USE_JOLT
struct PhysicsWorld::JoltState {
    JPH::TempAllocatorImpl* TempAllocator = nullptr;
    JPH::JobSystemThreadPool* JobSystem = nullptr;
    JPH::PhysicsSystem* System = nullptr;
    std::unordered_map<EntityID, JPH::BodyID> BodyMap;
    bool Registered = false;
};
#endif

PhysicsWorld::PhysicsWorld() = default;
PhysicsWorld::~PhysicsWorld() { Shutdown(); }

void PhysicsWorld::Initialize() {
#ifdef MUK_USE_JOLT
    InitJolt();
    if (m_Jolt && m_Jolt->System) {
        MUK_CORE_INFO("PhysicsWorld: using Jolt Physics");
        m_Initialized = true;
        return;
    }
    MUK_CORE_WARN("PhysicsWorld: Jolt init failed, falling back to simple solver");
    m_Jolt.reset();
#endif
    InitSimple();
    m_Initialized = true;
    MUK_CORE_INFO("PhysicsWorld: using simple CPU solver");
}

void PhysicsWorld::InitSimple() {
    m_SimpleBodies.clear();
    m_NextBodyId = 1;
}

void PhysicsWorld::InitJolt() {
#ifdef MUK_USE_JOLT
    m_Jolt = std::make_unique<JoltState>();

    JPH::RegisterDefaultAllocator();
    if (!m_Jolt->Registered) {
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        m_Jolt->Registered = true;
    }

    m_Jolt->TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
    int threads = static_cast<int>(std::thread::hardware_concurrency()) - 1;
    if (threads < 1) threads = 1;
    m_Jolt->JobSystem = new JPH::JobSystemThreadPool(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, threads);

    m_Jolt->System = new JPH::PhysicsSystem();
    m_Jolt->System->Init(1024, 0, 1024, 1024,
                         s_BPLayers, s_ObjectVsBroadPhase, s_ObjectVsObject);
    m_Jolt->System->SetGravity(JPH::Vec3(m_Gravity.x, m_Gravity.y, m_Gravity.z));
#endif
}

void PhysicsWorld::Shutdown() {
#ifdef MUK_USE_JOLT
    ShutdownJolt();
#endif
    m_SimpleBodies.clear();
    m_Initialized = false;
}

void PhysicsWorld::ShutdownJolt() {
#ifdef MUK_USE_JOLT
    if (!m_Jolt) return;
    if (m_Jolt->System) {
        for (auto& [id, bodyId] : m_Jolt->BodyMap) {
            m_Jolt->System->GetBodyInterface().RemoveBody(bodyId);
            m_Jolt->System->GetBodyInterface().DestroyBody(bodyId);
        }
        m_Jolt->BodyMap.clear();
    }
    delete m_Jolt->System;
    delete m_Jolt->JobSystem;
    delete m_Jolt->TempAllocator;
    m_Jolt.reset();
#endif
}

bool PhysicsWorld::IsUsingJolt() const {
#ifdef MUK_USE_JOLT
    return m_Jolt != nullptr && m_Jolt->System != nullptr;
#else
    return false;
#endif
}

void* PhysicsWorld::GetJoltSystemPtr() const {
#ifdef MUK_USE_JOLT
    return m_Jolt ? m_Jolt->System : nullptr;
#else
    return nullptr;
#endif
}

void* PhysicsWorld::GetJoltTempAllocatorPtr() const {
#ifdef MUK_USE_JOLT
    return m_Jolt ? m_Jolt->TempAllocator : nullptr;
#else
    return nullptr;
#endif
}

EntityID PhysicsWorld::CreateBody(const RigidBodyDesc& desc) {
#ifdef MUK_USE_JOLT
    if (IsUsingJolt()) {
        JPH::BodyInterface& bi = m_Jolt->System->GetBodyInterface();
        JPH::RefConst<JPH::Shape> shape;

        switch (desc.Shape) {
            case ShapeType::Box:
                shape = new JPH::BoxShape(JPH::Vec3(desc.HalfExtents.x, desc.HalfExtents.y, desc.HalfExtents.z));
                break;
            case ShapeType::Sphere:
                shape = new JPH::SphereShape(desc.Radius);
                break;
            case ShapeType::Capsule:
                shape = new JPH::CapsuleShape(desc.Height * 0.5f, desc.Radius);
                break;
            default:
                shape = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
                break;
        }

        JPH::EMotionType motion =
            desc.Type == BodyType::Static ? JPH::EMotionType::Static :
            desc.Type == BodyType::Kinematic ? JPH::EMotionType::Kinematic :
            JPH::EMotionType::Dynamic;

        JPH::ObjectLayer layer = (desc.Type == BodyType::Static) ? LAYER_NON_MOVING : LAYER_MOVING;

        JPH::BodyCreationSettings settings(
            shape,
            JPH::RVec3(desc.Position.x, desc.Position.y, desc.Position.z),
            JPH::Quat::sIdentity(),
            motion,
            layer
        );
        settings.mFriction = desc.Friction;
        settings.mRestitution = desc.Restitution;

        JPH::BodyID joltId = bi.CreateAndAddBody(settings, JPH::EActivation::Activate);
        EntityID id = m_NextBodyId++;
        m_Jolt->BodyMap[id] = joltId;
        return id;
    }
#endif
    SimpleBody body;
    body.Id = m_NextBodyId++;
    body.Desc = desc;
    body.Position = desc.Position;
    body.Velocity = {0, 0, 0};
    body.Active = true;
    m_SimpleBodies.push_back(body);
    return body.Id;
}

void PhysicsWorld::DestroyBody(EntityID body) {
#ifdef MUK_USE_JOLT
    if (IsUsingJolt()) {
        auto it = m_Jolt->BodyMap.find(body);
        if (it != m_Jolt->BodyMap.end()) {
            m_Jolt->System->GetBodyInterface().RemoveBody(it->second);
            m_Jolt->System->GetBodyInterface().DestroyBody(it->second);
            m_Jolt->BodyMap.erase(it);
        }
        return;
    }
#endif
    for (auto it = m_SimpleBodies.begin(); it != m_SimpleBodies.end(); ++it) {
        if (it->Id == body) {
            m_SimpleBodies.erase(it);
            return;
        }
    }
}

void PhysicsWorld::SetGravity(const Vec3& gravity) {
    m_Gravity = gravity;
#ifdef MUK_USE_JOLT
    if (IsUsingJolt())
        m_Jolt->System->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
#endif
}

Vec3 PhysicsWorld::GetGravity() const { return m_Gravity; }

void PhysicsWorld::Update(f32 deltaTime) {
    if (!m_Initialized) return;
#ifdef MUK_USE_JOLT
    if (IsUsingJolt()) {
        UpdateJolt(deltaTime);
        return;
    }
#endif
    UpdateSimple(deltaTime);
}

void PhysicsWorld::UpdateSimple(f32 deltaTime) {
    for (auto& body : m_SimpleBodies) {
        if (!body.Active || body.Desc.Type == BodyType::Static)
            continue;

        if (body.Desc.GravityEnabled)
            body.Velocity = body.Velocity + m_Gravity * deltaTime;

        body.Position = body.Position + body.Velocity * deltaTime;

        f32 groundY = (body.Desc.Shape == ShapeType::Sphere) ? body.Desc.Radius : body.Desc.HalfExtents.y;
        if (body.Position.y < groundY) {
            body.Position.y = groundY;
            body.Velocity.y = -body.Velocity.y * body.Desc.Restitution;
            body.Velocity.x *= (1.0f - body.Desc.Friction * deltaTime * 5.0f);
            body.Velocity.z *= (1.0f - body.Desc.Friction * deltaTime * 5.0f);
        }
    }
}

void PhysicsWorld::UpdateJolt(f32 deltaTime) {
#ifdef MUK_USE_JOLT
    m_Jolt->System->Update(deltaTime, 1, m_Jolt->TempAllocator, m_Jolt->JobSystem);
#else
    (void)deltaTime;
#endif
}

Vec3 PhysicsWorld::GetBodyPosition(EntityID body) const {
#ifdef MUK_USE_JOLT
    if (IsUsingJolt()) {
        auto it = m_Jolt->BodyMap.find(body);
        if (it != m_Jolt->BodyMap.end()) {
            JPH::RVec3 p = m_Jolt->System->GetBodyInterface().GetCenterOfMassPosition(it->second);
            return { (f32)p.GetX(), (f32)p.GetY(), (f32)p.GetZ() };
        }
        return {};
    }
#endif
    for (const auto& b : m_SimpleBodies)
        if (b.Id == body) return b.Position;
    return {};
}

void PhysicsWorld::SetBodyPosition(EntityID body, const Vec3& pos) {
#ifdef MUK_USE_JOLT
    if (IsUsingJolt()) {
        auto it = m_Jolt->BodyMap.find(body);
        if (it != m_Jolt->BodyMap.end()) {
            m_Jolt->System->GetBodyInterface().SetPosition(
                it->second, JPH::RVec3(pos.x, pos.y, pos.z), JPH::EActivation::Activate);
        }
        return;
    }
#endif
    for (auto& b : m_SimpleBodies)
        if (b.Id == body) { b.Position = pos; return; }
}

Vec3 PhysicsWorld::GetBodyVelocity(EntityID body) const {
#ifdef MUK_USE_JOLT
    if (IsUsingJolt()) {
        auto it = m_Jolt->BodyMap.find(body);
        if (it != m_Jolt->BodyMap.end()) {
            JPH::Vec3 v = m_Jolt->System->GetBodyInterface().GetLinearVelocity(it->second);
            return { v.GetX(), v.GetY(), v.GetZ() };
        }
        return {};
    }
#endif
    for (const auto& b : m_SimpleBodies)
        if (b.Id == body) return b.Velocity;
    return {};
}

} // namespace Muk
