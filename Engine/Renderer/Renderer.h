#pragma once

#include "Core/Core.h"
#include "RHI/RHI.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include <memory>
#include <string>

namespace Muk {

class DX12Pipeline;
class DX12SceneRT;

struct CameraView {
    Vec3 Eye{0.0f, 1.5f, -4.0f};
    Vec3 Target{0.0f, 0.0f, 0.0f};
    Vec3 Up{0.0f, 1.0f, 0.0f};
    f32 FOVDegrees = 60.0f;
    f32 Near = 0.1f;
    f32 Far = 1000.0f;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool Initialize(void* windowHandle, u32 width, u32 height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void OnResize(u32 width, u32 height);

    bool UploadMesh(const std::string& name, const Mesh& mesh);
    bool UploadTexture(const std::string& name, const Texture& texture);
    void SetCamera(const CameraView& camera);
    void SetDirectionalLight(const Vec3& dir, const Vec3& color, f32 intensity, f32 ambient);

    void DrawMesh(const std::string& meshName, const Mat4& world, const Material& material);
    void DrawMesh(const Mesh& mesh, const Mat4& transform, const Material& material);

    bool EnsureSceneRT(u32 width, u32 height);
    void BeginSceneRT();
    void EndSceneRT();
    void* GetSceneRTGpuHandle() const;
    u32 GetSceneRTWidth() const;
    u32 GetSceneRTHeight() const;

    RHI* GetRHI() const { return m_RHI.get(); }
    Mat4 GetViewProjection() const { return m_ViewProjection; }
    const CameraView& GetCamera() const { return m_Camera; }

private:
    void RebuildViewProjection();

    std::unique_ptr<RHI> m_RHI;
    std::unique_ptr<DX12Pipeline> m_Pipeline;
    std::unique_ptr<DX12SceneRT> m_SceneRT;
    CameraView m_Camera;
    Mat4 m_ViewProjection = Mat4::Identity();

    Vec3 m_LightDir{0.35f, -1.0f, 0.25f};
    Vec3 m_LightColor{1.0f, 0.98f, 0.92f};
    f32 m_LightIntensity = 1.2f;
    f32 m_Ambient = 0.18f;

    u32 m_Width = 1;
    u32 m_Height = 1;
    bool m_Initialized = false;
    bool m_RenderingToSceneRT = false;
};

} // namespace Muk
