#include "Renderer.h"
#include "Core/Log.h"
#include "RHI/DX12/DX12RHI.h"
#include "RHI/DX12/DX12Pipeline.h"

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

#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (dx12) {
        m_Pipeline = std::make_unique<DX12Pipeline>();
        if (!m_Pipeline->Initialize(dx12->GetDevice(), dx12->GetBackBufferFormat())) {
            MUK_CORE_ERROR("Failed to initialize DX12Pipeline");
            m_Pipeline.reset();
        }
    }
#endif

    m_Initialized = true;
    MUK_CORE_INFO("Renderer initialized");
    return true;
}

void Renderer::Shutdown() {
    if (m_Pipeline) {
        m_Pipeline->Shutdown();
        m_Pipeline.reset();
    }
    if (m_RHI) {
        m_RHI->Shutdown();
        m_RHI.reset();
    }
    m_Initialized = false;
    m_MeshUploaded = false;
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

bool Renderer::UploadMesh(const Mesh& mesh) {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_Pipeline) return false;

    if (m_Pipeline->UploadMesh(dx12->GetDevice(), dx12->GetCommandList(),
                               dx12->GetCommandQueue(), mesh)) {
        m_MeshUploaded = true;
        return true;
    }
#endif
    return false;
}

void Renderer::DrawMesh(const Mesh& mesh, const Mat4& transform, const Material& material) {
    (void)material;

#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_Pipeline || !m_Pipeline->IsReady()) return;

    // Auto-upload on first draw if needed
    if (!m_MeshUploaded) {
        UploadMesh(mesh);
    }

    auto* cmdList = dx12->GetCommandList();
    m_Pipeline->Bind(cmdList);
    m_Pipeline->SetMVP(cmdList, transform);
    m_Pipeline->Draw(cmdList);
#else
    (void)mesh;
    (void)transform;
#endif
}

} // namespace Muk
