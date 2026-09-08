#include "Engine.h"

using namespace Muk;

class SandboxApp : public Application {
protected:
    void OnInit() override {
        // Camera looking at origin
        CameraView cam;
        cam.Eye = {0.0f, 1.2f, -3.5f};
        cam.Target = {0.0f, 0.0f, 0.0f};
        cam.FOVDegrees = 60.0f;
        Renderer().SetCamera(cam);

        // Multi-mesh GPU cache
        if (auto tri = Assets().GetMesh("Triangle"))
            Renderer().UploadMesh("Triangle", *tri);
        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);

        RigidBodyDesc desc;
        desc.Type = BodyType::Dynamic;
        desc.Shape = ShapeType::Box;
        desc.Position = {0.0f, 5.0f, 0.0f};
        desc.HalfExtents = {0.5f, 0.5f, 0.5f};
        desc.Restitution = 0.3f;
        m_PhysicsBody = Physics().CreateBody(desc);

        MUK_CORE_INFO("Sandbox: depth + camera MVP + multi-mesh cache ready");
    }

    void OnUpdate(float deltaTime) override {
        m_Time += deltaTime;
        (void)m_PhysicsBody;
    }

    void OnRender() override {
        auto mat = Assets().GetMaterial("Default");
        if (!mat) return;

        // Triangle slightly in front
        Mat4 triWorld = Mat4::Translation({-0.8f, 0.0f, 0.0f}) * Mat4::Scale({0.6f, 0.6f, 0.6f});
        Renderer().DrawMesh("Triangle", triWorld, *mat);

        // Cube to the right
        Mat4 cubeWorld = Mat4::Translation({0.9f, 0.0f, 0.0f}) * Mat4::Scale({0.5f, 0.5f, 0.5f});
        Renderer().DrawMesh("Cube", cubeWorld, *mat);
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
