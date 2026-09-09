/**
 * Muk Game Engine — real Windows application (not a stub).
 * Opens a DX12 window, draws a lit 3D scene (floor + cubes).
 */
#include "Engine.h"
#include <cmath>
#include <iostream>

using namespace Muk;

class MukEditorApp : public Application {
protected:
    void OnInit() override {
        std::cout << "Muk Game Engine Editor v0.15\n";
        MUK_CORE_INFO("Starting real DX12 editor application");

        CameraView cam;
        cam.Eye = { 4.0f, 3.5f, -7.0f };
        cam.Target = { 0.0f, 0.5f, 0.0f };
        cam.FOVDegrees = 60.0f;
        Renderer().SetCamera(cam);

        // Floor
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Floor";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = { 0, -0.05f, 0 };
            t.Scale = { 12.f, 0.1f, 12.f };
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
        }
        // Cubes
        const Vec3 positions[] = {
            { 0, 0.5f, 0 }, { 2, 0.5f, 1 }, { -2, 0.5f, -1 }, { 1.5f, 1.5f, -2 }
        };
        for (int i = 0; i < 4; ++i) {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Cube" + std::to_string(i);
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = positions[i];
            t.Scale = { 1, 1, 1 };
            ECS().AddComponent<MeshRenderer>(e).MeshName = "Cube";
        }

        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);
        if (auto tri = Assets().GetMesh("Triangle"))
            Renderer().UploadMesh("Triangle", *tri);

        MUK_CORE_INFO("Scene ready — close the window to exit");
        std::cout << "Window title: Muk Game Engine\n";
        std::cout << "You should see a 3D floor and cubes.\n";
    }

    void OnUpdate(float dt) override {
        m_Time += dt;
        // Slow orbit of the camera around the scene
        float r = 8.0f;
        float angle = m_Time * 0.25f;
        CameraView cam = Renderer().GetCamera();
        cam.Eye = { std::sin(angle) * r, 3.5f, std::cos(angle) * r };
        cam.Target = { 0.0f, 0.5f, 0.0f };
        Renderer().SetCamera(cam);
    }

    void OnRender() override {
        Renderer().SetDirectionalLight(
            { 0.45f, -1.0f, 0.35f },
            { 1.0f, 0.98f, 0.92f },
            1.4f,
            0.22f
        );

        ECS().ForEach<Transform, MeshRenderer>([&](Entity, Transform& t, MeshRenderer& mr) {
            if (!mr.Visible) return;
            Material mat = Material::CreateDefault();
            if (mr.MeshName == "Cube" && t.Scale.y < 0.2f) {
                // floor tint
                mat.BaseColor = { 0.35f, 0.38f, 0.42f, 1.0f };
            } else {
                mat.BaseColor = { 0.85f, 0.55f, 0.25f, 1.0f };
            }
            Renderer().DrawMesh(mr.MeshName, t.GetMatrix(), mat);
        });
    }

    void OnShutdown() override {
        MUK_CORE_INFO("Editor closed");
    }

private:
    float m_Time = 0.f;
};

int main() {
    MukEditorApp app;
    app.Run();
    return 0;
}
