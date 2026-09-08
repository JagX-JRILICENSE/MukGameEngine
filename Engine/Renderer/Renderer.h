#pragma once

#include "Core/Core.h"
#include "RHI/RHI.h"
#include "Mesh.h"
#include "Material.h"
#include "Math/Matrix.h"
#include <memory>

namespace Muk {

class DX12Pipeline; // forward

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool Initialize(void* windowHandle, u32 width, u32 height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void OnResize(u32 width, u32 height);

    // Upload mesh once, then draw each frame with transform
    bool UploadMesh(const Mesh& mesh);
    void DrawMesh(const Mesh& mesh, const Mat4& transform, const Material& material);

    RHI* GetRHI() const { return m_RHI.get(); }

private:
    std::unique_ptr<RHI> m_RHI;
    std::unique_ptr<DX12Pipeline> m_Pipeline;
    bool m_Initialized = false;
    bool m_MeshUploaded = false;
};

} // namespace Muk
