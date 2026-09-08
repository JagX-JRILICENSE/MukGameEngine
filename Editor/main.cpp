#include "Engine.h"
#include "EditorUI/EditorUI.h"
#include "AI/AIControlPanel.h"
#include "Scene/SceneRenderer.h"
#include "Core/Profiler.h"
#include "RHI/DX12/DX12RHI.h"

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

using namespace Muk;

class EditorApp : public Application {
protected:
    void OnInit() override {
        auto* dx = dynamic_cast<DX12RHI*>(Renderer().GetRHI());
        m_UI.Initialize(Window().GetNativeHandle(), dx);
        m_AI.Initialize();

        CameraView cam;
        cam.Eye = {0.0f, 2.5f, -6.0f};
        cam.Target = {0.0f, 0.5f, 0.0f};
        Renderer().SetCamera(cam);

        // Light
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Sun";
            auto& L = ECS().AddComponent<DirectionalLight>(e);
            L.Direction = {0.4f, -1.0f, 0.3f};
            L.Intensity = 1.4f;
            L.Ambient = 0.2f;
            Track(e, "Sun");
        }

        // Floor
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Floor";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {0, -0.5f, 0};
            t.Scale = {8, 0.2f, 8};
            auto& mr = ECS().AddComponent<MeshRenderer>(e);
            mr.MeshName = "Cube";
            mr.MaterialName = "Default";
            Track(e, "Floor");
        }

        // Hero cube
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Cube";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {0, 0.5f, 0};
            auto& mr = ECS().AddComponent<MeshRenderer>(e);
            mr.MeshName = "Cube";
            mr.MaterialName = "Default";
            Track(e, "Cube");
            m_Selected = e;
        }

        // Textured triangle prop
        {
            auto e = ECS().CreateEntity();
            ECS().AddComponent<NameComponent>(e).Name = "Triangle";
            auto& t = ECS().AddComponent<Transform>(e);
            t.Position = {-2, 0.5f, 0};
            auto& mr = ECS().AddComponent<MeshRenderer>(e);
            mr.MeshName = "Triangle";
            mr.MaterialName = "CheckerMat";
            Track(e, "Triangle");
        }

        if (auto tri = Assets().GetMesh("Triangle"))
            Renderer().UploadMesh("Triangle", *tri);
        if (auto cube = Assets().GetMesh("Cube"))
            Renderer().UploadMesh("Cube", *cube);

        auto checker = Texture::CreateSolid(64, 64, 200, 180, 60);
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

        Material cm = Material::CreateDefault();
        cm.Name = "CheckerMat";
        cm.AlbedoMap = checker;
        cm.AlbedoTexture = "Checker";
        cm.Roughness = 0.6f;
        // Register in asset manager if API exists — keep local fallback in SceneRenderer via name
        m_CheckerMat = cm;

        m_UI.Log("Phase A: lit ECS scene + gizmos + profiler + AI actions");
    }

    void OnUpdate(float) override {}

    void OnRender() override {
        m_UI.BeginFrame();
        m_UI.DrawDockspace();
        m_UI.DrawHierarchy(m_Entities, m_Selected);
        DrawDetailsWithGizmo();
        m_UI.DrawContentBrowser();
        m_UI.DrawConsole();
        m_AI.Draw(Renderer());
        DrawStats();

        // Inject checker material lookup: temporary draw path uses SceneRenderer + manual override
        if (auto* mr = ECS().GetComponent<MeshRenderer>(m_Entities.size() > 3 ? m_Entities[3].Handle : Entity{})) {
            (void)mr;
        }

        u32 vw = m_UI.GetDesiredViewportWidth();
        u32 vh = m_UI.GetDesiredViewportHeight();
        Renderer().EnsureSceneRT(vw, vh);
        Renderer().BeginSceneRT();

        // Custom draw so CheckerMat works without full material registry
        Renderer().SetDirectionalLight({0.4f, -1.0f, 0.3f}, {1, 0.98f, 0.92f}, 1.4f, 0.2f);
        ECS().ForEach<Transform, MeshRenderer>([&](Entity e, Transform& t, MeshRenderer& mr) {
            if (!mr.Visible) return;
            Material mat = Material::CreateDefault();
            if (mr.MaterialName == "CheckerMat")
                mat = m_CheckerMat;
            else if (auto m = Assets().GetMaterial(mr.MaterialName))
                mat = *m;
            Renderer().DrawMesh(mr.MeshName, t.GetMatrix(), mat);
            (void)e;
        });

        Renderer().EndSceneRT();

        m_UI.DrawViewport(Renderer(), Renderer().GetSceneRTGpuHandle(),
                          Renderer().GetSceneRTWidth(), Renderer().GetSceneRTHeight());

        m_UI.RenderDrawData();
        m_UI.EndFrame();
    }

    void OnShutdown() override { m_UI.Shutdown(); }

private:
    void Track(Entity e, const std::string& name) {
        EditorEntityInfo info;
        info.Handle = e;
        info.Name = name;
        m_Entities.push_back(info);
        m_UI.Log("Created: " + name);
    }

    void DrawDetailsWithGizmo() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Details");
        if (m_Selected.IsValid()) {
            ImGui::Text("Entity %u", m_Selected.GetID());
            if (auto* t = ECS().GetComponent<Transform>(m_Selected)) {
                ImGui::Separator();
                ImGui::TextUnformatted("Transform (gizmo)");
                ImGui::DragFloat3("Position", &t->Position.x, 0.05f);
                ImGui::DragFloat3("Rotation", &t->Rotation.x, 0.5f);
                ImGui::DragFloat3("Scale", &t->Scale.x, 0.05f);
            }
            if (auto* mr = ECS().GetComponent<MeshRenderer>(m_Selected)) {
                ImGui::Separator();
                ImGui::Checkbox("Visible", &mr->Visible);
                ImGui::Text("Mesh: %s", mr->MeshName.c_str());
            }
        } else {
            ImGui::TextDisabled("No selection");
        }
        ImGui::End();
#else
        m_UI.DrawDetails(ECS(), m_Selected);
#endif
    }

    void DrawStats() {
#ifdef MUK_USE_IMGUI
        ImGui::Begin("Stats");
        auto& p = Profiler::Get();
        ImGui::Text("FPS: %.1f", p.Fps());
        ImGui::Text("Frame: %.2f ms", p.LastFrameMs());
        for (auto& [name, ms] : p.LastScopes())
            ImGui::Text("%s: %.2f ms", name.c_str(), ms);
        ImGui::End();
#endif
    }

    EditorUI m_UI;
    AIControlPanel m_AI;
    SceneRenderer m_SceneDraw;
    std::vector<EditorEntityInfo> m_Entities;
    Entity m_Selected;
    Material m_CheckerMat;
};

int main() {
    EditorApp app;
    app.Run();
    return 0;
}
