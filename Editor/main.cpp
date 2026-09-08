#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "RHI/DX12/DX12RHI.h"

using namespace Muk;

class EditorApp : public Application {
protected:
    void OnInit() override {
        auto* dx = dynamic_cast<DX12RHI*>(Renderer().GetRHI());
        m_UI.Initialize(Window().GetNativeHandle(), dx);

        CameraView cam;
        cam.Eye = {0.0f, 2.0f, -5.0f};
        cam.Target = {0.0f, 0.0f, 0.0f};
        Renderer().SetCamera(cam);

        CreateEditorEntity("Main Camera", true);
        CreateEditorEntity("Directional Light", false);
        CreateEditorEntity("Floor", false);

        if (auto tri = Assets().GetMesh("Triangle"))
            Renderer().UploadMesh("Triangle", *tri);
        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);

        // Demo textured material using solid CPU texture uploaded to GPU
        auto checker = Texture::CreateSolid(64, 64, 200, 180, 60);
        // Simple checker pattern
        for (u32 y = 0; y < 64; ++y)
            for (u32 x = 0; x < 64; ++x) {
                bool c = ((x / 8) + (y / 8)) & 1;
                u32 i = (y * 64 + x) * 4;
                checker->Pixels[i+0] = c ? 220 : 40;
                checker->Pixels[i+1] = c ? 180 : 40;
                checker->Pixels[i+2] = c ? 40 : 120;
                checker->Pixels[i+3] = 255;
            }
        checker->Name = "Checker";
        Renderer().UploadTexture("Checker", *checker);
        m_TexMat = Material::CreateDefault();
        m_TexMat.Name = "CheckerMat";
        m_TexMat.AlbedoMap = checker;
        m_TexMat.AlbedoTexture = "Checker";

        m_UI.Log("SceneRT viewport + GPU albedo sampling enabled");
    }

    void OnUpdate(float) override {}

    void OnRender() override {
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        m_UI.DrawDetails(ECS(), m_Selected);
        m_UI.DrawContentBrowser();
        m_UI.DrawConsole();

        // Resize RTT to viewport panel size
        u32 vw = m_UI.GetDesiredViewportWidth();
        u32 vh = m_UI.GetDesiredViewportHeight();
        Renderer().EnsureSceneRT(vw, vh);

        // Draw scene into offscreen target
        Renderer().BeginSceneRT();
        Renderer().DrawMesh("Triangle",
            Mat4::Translation({-0.8f, 0, 0}) * Mat4::Scale({0.5f, 0.5f, 0.5f}), m_TexMat);
        auto def = Assets().GetMaterial("Default");
        if (def)
            Renderer().DrawMesh("Cube",
                Mat4::Translation({0.9f, 0, 0}) * Mat4::Scale({0.4f, 0.4f, 0.4f}), *def);
        Renderer().EndSceneRT();

        // Show RTT inside ImGui Viewport panel
        m_UI.DrawViewport(Renderer(), Renderer().GetSceneRTGpuHandle(),
                          Renderer().GetSceneRTWidth(), Renderer().GetSceneRTHeight());

        m_UI.RenderDrawData();
        m_UI.EndFrame();
    }

    void OnShutdown() override { m_UI.Shutdown(); }

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
    Material m_TexMat;
};

int main() {
    EditorApp app;
    app.Run();
    return 0;
}
