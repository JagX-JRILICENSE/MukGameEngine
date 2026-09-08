#include "RHI.h"
#include "Core/Log.h"

#ifdef MUK_RHI_DX12
#include "DX12/DX12RHI.h"
#endif

namespace Muk {

class NullRHI : public RHI {
public:
    bool Initialize(void* nativeWindowHandle, u32 width, u32 height) override {
        MUK_CORE_INFO("NullRHI initialized (no graphics backend)");
        return true;
    }
    void Shutdown() override {}
    void BeginFrame() override {}
    void EndFrame() override {}
    void Resize(u32 width, u32 height) override {}
};

std::unique_ptr<RHI> RHI::Create() {
#ifdef MUK_RHI_DX12
    return std::make_unique<DX12RHI>();
#else
    return std::make_unique<NullRHI>();
#endif
}

} // namespace Muk
