#include "Renderer.h"
#include "Core/Log.h"

namespace Muk {

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize(void* windowHandle, u32 width, u32 height) {
    m_RHI = RHI::Create();
    if (!m_RHI || !m_RHI->Initialize(windowHandle, width, height)) {
        MUK_CORE_ERROR("Failed to initialize RHI");
        return false;
    }
    m_Initialized = true;
    MUK_CORE_INFO("Renderer initialized (DX12)");
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

void Renderer::DrawMesh(const Mesh& mesh, const Mat4& transform, const Material& material) {
    // Placeholder: real GPU buffer upload + PSO + draw call will come next
    // For now the clear color from DX12RHI::BeginFrame is visible.
    (void)mesh;
    (void)transform;
    (void)material;
}

} // namespace Muk
