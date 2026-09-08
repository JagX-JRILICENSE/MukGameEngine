#include "Engine.h"
#include <iostream>

using namespace Muk;

class SandboxApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("Sandbox application initialized");

        // Create a simple entity with transform
        auto entity = ECS().CreateEntity();
        auto& transform = ECS().AddComponent<Transform>(entity);
        transform.Position = {0.0f, 2.0f, 0.0f};

        // Spawn a dynamic physics body that will fall
        RigidBodyDesc desc;
        desc.Type = BodyType::Dynamic;
        desc.Shape = ShapeType::Box;
        desc.Position = {0.0f, 5.0f, 0.0f};
        desc.HalfExtents = {0.5f, 0.5f, 0.5f};
        desc.Mass = 1.0f;
        desc.Restitution = 0.3f;
        m_PhysicsBody = Physics().CreateBody(desc);

        MUK_CORE_INFO("Created test physics body that will fall under gravity");
    }

    void OnUpdate(float deltaTime) override {
        // Sync physics position back (demo)
        if (m_PhysicsBody != 0) {
            Vec3 pos = Physics().GetBodyPosition(m_PhysicsBody);
            // In a real engine we would write this back to the Transform component
            (void)pos;
        }
    }

    void OnRender() override {
        // Draw a triangle (GPU path still being completed)
        auto mesh = Assets().GetMesh("Triangle");
        auto material = Assets().GetMaterial("Default");
        if (mesh && material) {
            Mat4 transform = Mat4::Identity();
            Renderer().DrawMesh(*mesh, transform, *material);
        }
    }

    void OnShutdown() override {
        MUK_CORE_INFO("Sandbox application shutting down");
    }

private:
    EntityID m_PhysicsBody = 0;
};

int main() {
    SandboxApp app;
    app.Run();
    return 0;
}
