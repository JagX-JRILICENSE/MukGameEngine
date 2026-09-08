#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "RHI/DX12/DX12RHI.h"

using namespace Muk;

class EditorApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("Muk Editor v0.3");

        auto* dx = dynamic_cast<DX12RHI*>(Renderer().GetRHI());
        m_UI.Initialize(Window().GetNativeHandle(), dx);

        CameraView cam;
        cam.Eye = {0.0f, 2.0f, -5.0f};
        cam.Target = {0.0f, 0.0f, 0.0f};
        Renderer().SetCamera(cam);

        CreateEditorEntity("Main Camera", true);
        CreateEditorEntity("Directional Light", false);
        CreateEditorEntity("Floor", false);
        CreateEditorEntity("Player Start", false);

        if (auto tri = Assets().GetMesh("Triangle"))
            Renderer().UploadMesh("Triangle", *tri);
        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);

        RigidBodyDesc desc;
        desc.Type = BodyType::Dynamic;
        desc.Shape = ShapeType::Sphere;
        desc.Position = {0.0f, 8.0f, 0.0f};
        desc.Radius = 0.5f;
        desc.Restitution = 0.6f;
        Physics().CreateBody(desc);

        m_UI.Log("ImGui DX12 font SRV + depth + camera MVP + mesh cache online");
        m_UI.Log(Physics().IsUsingJolt() ? "Physics: Jolt" : "Physics: simple");
    }

    void OnUpdate(float) override {}

    void OnRender() override {
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        m_UI.DrawDetails(ECS(), m_Selected);
        m_UI.DrawViewportPlaceholder();
        m_UI.DrawContentBrowser();
        m_UI.DrawConsole();

        auto mat = Assets().GetMaterial("Default");
        if (mat) {
            Renderer().DrawMesh("Triangle", Mat4::Translation({-0.7f, 0, 0}) * Mat4::Scale({0.5f, 0.5f, 0.5f}), *mat);
            Renderer().DrawMesh("Cube", Mat4::Translation({0.8f, 0, 0}) * Mat4::Scale({0.4f, 0.4f, 0.4f}), *mat);
        }

        // ImGui on top of scene into same command list
        m_UI.RenderDrawData();
        m_UI.EndFrame();
    }

    void OnShutdown() override {
        m_UI.Shutdown();
    }

private:
    void CreateEditorEntity(const std::string& name, bool withCamera) {
        EditorEntityInfo e;
        e.Handle = ECS().CreateEntity();
        e.Name = name;
        ECS().AddComponent<Transform>(e.Handle);
        if (withCamera) ECS().AddComponent<Camera>(e.Handle);
        m_Entities.push_back(e);
        m_UI.Log("Created: " + name);
    }

    EditorUI m_UI;
    std::vector<EditorEntityInfo> m_Entities;
    Entity m_Selected;
};

int main() {
    EditorApp app;
    app.Run();
    return 0;
}
