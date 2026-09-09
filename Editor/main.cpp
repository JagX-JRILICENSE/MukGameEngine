#include "Engine.h"
#include "Core/Log.h"
#include <iostream>

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>
#endif

using namespace Muk;

class EditorApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("Muk Game Engine Editor starting");
        Renderer().SetCamera({ {0.0f, 3.0f, -8.0f}, {0.0f, 0.5f, 0.0f} });

        auto floor = ECS().CreateEntity();
        ECS().AddComponent<NameComponent>(floor).Name = "Floor";
        auto& ft = ECS().AddComponent<Transform>(floor);
        ft.Position = {0, -0.1f, 0};
        ft.Scale = {10, 0.2f, 10};
        ECS().AddComponent<MeshRenderer>(floor).MeshName = "Cube";

        auto cube = ECS().CreateEntity();
        ECS().AddComponent<NameComponent>(cube).Name = "Cube";
        auto& ct = ECS().AddComponent<Transform>(cube);
        ct.Position = {0, 0.5f, 0};
        ECS().AddComponent<MeshRenderer>(cube).MeshName = "Cube";

        if (auto m = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *m);

        MUK_CORE_INFO("Scene ready — close window to exit");
    }

    void OnUpdate(float) override {}

    void OnRender() override {
        Renderer().SetDirectionalLight({0.4f, -1.0f, 0.3f}, {1, 0.98f, 0.95f}, 1.2f, 0.2f);
        ECS().ForEach<Transform, MeshRenderer>([&](Entity, Transform& t, MeshRenderer& mr) {
            if (!mr.Visible) return;
            Material mat = Material::CreateDefault();
            Renderer().DrawMesh(mr.MeshName, t.GetMatrix(), mat);
        });
    }

    void OnShutdown() override {
        MUK_CORE_INFO("Shutdown");
    }
};

int main() {
    std::cout << "Muk Game Engine v0.14 (Windows)" << std::endl;
    EditorApp app;
    app.Run();
    return 0;
}
