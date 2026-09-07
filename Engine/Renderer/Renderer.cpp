#include "Renderer.h"
#include "Core/Log.h"

namespace Muk {

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize(void* windowHandle, u32 width, u32 height) {
    m_RHI = RHI::Create();
    if (!m_RHI->Initialize(windowHandle, width, height)) {
        MUK_CORE_ERROR("Failed to initialize RHI");
        return false;
    }
    m_Initialized = true;
    MUK_CORE_INFO("Renderer initialized");
    return true;
}

void Renderer::Shutdown() {
    if (m_RHI) {
        m_RHI->Shutdown();
        m_RHI.reset();
    }
    m_Initialized = false;
}

void Renderer::BeginFrame() {
    if (m_RHI) m_RHI->BeginFrame();
}

void Renderer::EndFrame() {
    if (m_RHI) m_RHI->EndFrame();
}

void Renderer::OnResize(u32 width, u32 height) {
    if (m_RHI) m_RHI->Resize(width, height);
}

} // namespace Muk
