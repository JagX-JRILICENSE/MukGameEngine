#pragma once

#include "Core/Core.h"

namespace Muk {

/**
 * Rendering Hardware Interface
 * Abstracts DX12 / Vulkan / future backends
 */
class RHI {
public:
    virtual ~RHI() = default;

    virtual bool Initialize(void* nativeWindowHandle, u32 width, u32 height) = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Resize(u32 width, u32 height) = 0;

    static std::unique_ptr<RHI> Create();
};

} // namespace Muk
