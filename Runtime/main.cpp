#include "Engine.h"

using namespace Muk;

class SandboxApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("Sandbox: uploading triangle mesh to GPU");

        auto mesh = Assets().GetMesh("Triangle");
        if (mesh) {
            Renderer().UploadMesh(*mesh);
        }

        // Physics demo body
        RigidBodyDesc desc;
        desc.Type = BodyType::Dynamic;
        desc.Shape = ShapeType::Box;
        desc.Position = {0.0f, 5.0f, 0.0f};
        desc.HalfExtents = {0.5f, 0.5f, 0.5f};
        desc.Mass = 1.0f;
        desc.Restitution = 0.3f;
        m_PhysicsBody = Physics().CreateBody(desc);

        MUK_CORE_INFO("Sandbox ready - you should see a colored triangle");
    }

    void OnUpdate(float deltaTime) override {
        m_Time += deltaTime;
        (void)m_PhysicsBody;
    }

    void OnRender() override {
        auto mesh = Assets().GetMesh("Triangle");
        auto material = Assets().GetMaterial("Default");
        if (!mesh || !material) return;

        // Simple orthographic-ish scale so triangle is visible in NDC
        Mat4 transform = Mat4::Scale({0.8f, 0.8f, 0.8f});
        Renderer().DrawMesh(*mesh, transform, *material);
    }

    void OnShutdown() override {
        MUK_CORE_INFO("Sandbox shut down");
    }

private:
    EntityID m_PhysicsBody = 0;
    float m_Time = 0.0f;
};

int main() {
    SandboxApp app;
    app.Run();
    return 0;
}
