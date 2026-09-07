#pragma once

#include "Core/Core.h"
#include "RHI/RHI.h"
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

    // Future: Submit meshes, draw calls, etc.

private:
    std::unique_ptr<RHI> m_RHI;
    bool m_Initialized = false;
};

} // namespace Muk
