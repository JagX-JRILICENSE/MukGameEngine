#pragma once

#include "Core/Core.h"
#include "RHI/RHI.h"
#include "Mesh.h"
#include "Material.h"
#include "Math/Matrix.h"
#include <memory>

namespace Muk {

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool Initialize(void* windowHandle, u32 width, u32 height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void OnResize(u32 width, u32 height);

    // Simple draw submission (will expand to proper render graph later)
    void DrawMesh(const Mesh& mesh, const Mat4& transform, const Material& material);

    RHI* GetRHI() const { return m_RHI.get(); }

private:
    std::unique_ptr<RHI> m_RHI;
    bool m_Initialized = false;
};

} // namespace Muk
