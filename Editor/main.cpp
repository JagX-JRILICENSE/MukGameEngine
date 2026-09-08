#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "RHI/DX12/DX12RHI.h"

using namespace Muk;

class EditorApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("Muk Editor v0.2");

        void* device = nullptr;
        void* queue = nullptr;
#ifdef MUK_RHI_DX12
        if (auto* dx = dynamic_cast<DX12RHI*>(Renderer().GetRHI())) {
            device = dx->GetDevice();
            queue = dx->GetCommandQueue();
        }
#endif
        m_UI.Initialize(Window().GetNativeHandle(), device, queue);

        CreateEditorEntity("Main Camera", true);
        CreateEditorEntity("Directional Light", false);
        CreateEditorEntity("Floor", false);
        CreateEditorEntity("Player Start", false);

        auto mesh = Assets().GetMesh("Triangle");
        if (mesh) Renderer().UploadMesh(*mesh);

        RigidBodyDesc desc;
        desc.Type = BodyType::Dynamic;
        desc.Shape = ShapeType::Sphere;
        desc.Position = {0.0f, 8.0f, 0.0f};
        desc.Radius = 0.5f;
        desc.Restitution = 0.6f;
        Physics().CreateBody(desc);

        m_UI.Log("Editor ready. Hierarchy / Details / Viewport / Content / Console panels active.");
        if (Physics().IsUsingJolt())
            m_UI.Log("Physics backend: Jolt");
        else
            m_UI.Log("Physics backend: simple solver");
    }

    void OnUpdate(float deltaTime) override {
        (void)deltaTime;
    }

    void OnRender() override {
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        m_UI.DrawDetails(ECS(), m_Selected);
        m_UI.DrawViewportPlaceholder();
        m_UI.DrawContentBrowser();
        m_UI.DrawConsole();

        // Scene draw (main swapchain)
        auto mesh = Assets().GetMesh("Triangle");
        auto mat = Assets().GetMaterial("Default");
        if (mesh && mat) {
            Renderer().DrawMesh(*mesh, Mat4::Scale({0.7f, 0.7f, 0.7f}), *mat);
        }

        m_UI.EndFrame();
    }

    void OnShutdown() override {
        m_UI.Shutdown();
        MUK_CORE_INFO("Muk Editor closed");
    }

private:
    void CreateEditorEntity(const std::string& name, bool withCamera) {
        EditorEntityInfo e;
        e.Handle = ECS().CreateEntity();
        e.Name = name;
        ECS().AddComponent<Transform>(e.Handle);
        if (withCamera)
            ECS().AddComponent<Camera>(e.Handle);
        m_Entities.push_back(e);
        m_UI.Log("Created entity: " + name);
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
