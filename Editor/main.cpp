#include "Engine.h"
#include <iostream>
#include <string>
#include <vector>

using namespace Muk;

/**
 * Muk Editor
 *
 * Current status: Functional engine host with placeholder UI panels.
 * Next: Integrate Dear ImGui for real docking panels (Viewport, Hierarchy, Details, Content Browser).
 *
 * Planned layout (Unreal-style):
 * ┌─────────────────────────────────────────────────────────────┐
 * │  Menu Bar                                                   │
 * ├──────────────┬──────────────────────────────┬───────────────┤
 * │  Hierarchy   │         Viewport             │   Details     │
 * │  (entities)  │     (3D scene view)          │  (inspector)  │
 * ├──────────────┴──────────────────────────────┴───────────────┤
 * │  Content Browser / Console                                  │
 * └─────────────────────────────────────────────────────────────┘
 */

struct EditorEntity {
    Entity Handle;
    std::string Name;
};

class EditorApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("========================================");
        MUK_CORE_INFO("       Muk Editor v0.1 (Foundation)    ");
        MUK_CORE_INFO("========================================");
        MUK_CORE_INFO("Panels planned:");
        MUK_CORE_INFO("  - Viewport (3D scene rendering)");
        MUK_CORE_INFO("  - Hierarchy (entity tree)");
        MUK_CORE_INFO("  - Details (component inspector)");
        MUK_CORE_INFO("  - Content Browser");
        MUK_CORE_INFO("  - Console / Output Log");
        MUK_CORE_INFO("  - Visual Scripting (future)");
        MUK_CORE_INFO("========================================");

        // Seed a few example entities for the hierarchy
        CreateEditorEntity("Main Camera");
        CreateEditorEntity("Directional Light");
        CreateEditorEntity("Floor");
        CreateEditorEntity("Player Start");

        // Also create a physics test body
        RigidBodyDesc desc;
        desc.Type = BodyType::Dynamic;
        desc.Shape = ShapeType::Sphere;
        desc.Position = {0.0f, 8.0f, 0.0f};
        desc.Radius = 0.5f;
        desc.Restitution = 0.6f;
        Physics().CreateBody(desc);
    }

    void OnUpdate(float deltaTime) override {
        // Future: handle editor camera, gizmo interaction, selection, etc.
        m_FrameCount++;
        if (m_FrameCount % 120 == 0) {
            // Occasional status
            MUK_CORE_TRACE("Editor running... entities: {0}", (int)m_Entities.size());
        }
    }

    void OnRender() override {
        // Viewport clear is handled by DX12RHI
        // Future: render selected camera view into an ImGui image
        auto mesh = Assets().GetMesh("Cube");
        auto mat = Assets().GetMaterial("Default");
        if (mesh && mat) {
            Renderer().DrawMesh(*mesh, Mat4::Identity(), *mat);
        }
    }

    void OnShutdown() override {
        MUK_CORE_INFO("Muk Editor closed");
    }

private:
    void CreateEditorEntity(const std::string& name) {
        EditorEntity e;
        e.Handle = ECS().CreateEntity();
        e.Name = name;
        ECS().AddComponent<Transform>(e.Handle);
        m_Entities.push_back(e);
        MUK_CORE_INFO("Editor: Created entity '{0}'", name.c_str());
    }

    std::vector<EditorEntity> m_Entities;
    u32 m_FrameCount = 0;
};

int main() {
    EditorApp app;
    app.Run();
    return 0;
}
