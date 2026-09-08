#include "Renderer.h"
#include "Core/Log.h"
#include "Math/Math.h"
#include "RHI/DX12/DX12RHI.h"
#include "RHI/DX12/DX12Pipeline.h"

namespace Muk {

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize(void* windowHandle, u32 width, u32 height) {
    m_Width = width;
    m_Height = height;

    m_RHI = RHI::Create();
    if (!m_RHI || !m_RHI->Initialize(windowHandle, width, height)) {
        MUK_CORE_ERROR("Failed to initialize RHI");
        return false;
    }

#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (dx12) {
        m_Pipeline = std::make_unique<DX12Pipeline>();
        if (!m_Pipeline->Initialize(dx12->GetDevice(), dx12->GetBackBufferFormat(), dx12->GetDepthFormat())) {
            MUK_CORE_ERROR("Failed to initialize DX12Pipeline");
            m_Pipeline.reset();
        }
    }
#endif

    RebuildViewProjection();
    m_Initialized = true;
    MUK_CORE_INFO("Renderer initialized (depth + camera MVP)");
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
}

void Renderer::BeginFrame() {
    if (m_RHI) m_RHI->BeginFrame();
}

void Renderer::EndFrame() {
    if (m_RHI) m_RHI->EndFrame();
}

void Renderer::OnResize(u32 width, u32 height) {
    m_Width = width;
    m_Height = height;
    if (m_RHI) m_RHI->Resize(width, height);
    RebuildViewProjection();
}

void Renderer::SetCamera(const CameraView& camera) {
    m_Camera = camera;
    RebuildViewProjection();
}

void Renderer::RebuildViewProjection() {
    f32 aspect = (m_Height > 0) ? (f32)m_Width / (f32)m_Height : 1.0f;
    Mat4 view = Mat4::LookAt(m_Camera.Eye, m_Camera.Target, m_Camera.Up);
    Mat4 proj = Mat4::Perspective(ToRadians(m_Camera.FOVDegrees), aspect, m_Camera.Near, m_Camera.Far);
    m_ViewProjection = proj * view; // column-major: P * V * W applied as W then V then P in shader mul
}

bool Renderer::UploadMesh(const std::string& name, const Mesh& mesh) {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_Pipeline) return false;
    return m_Pipeline->UploadMesh(dx12->GetDevice(), name, mesh);
#else
    (void)name; (void)mesh;
    return false;
#endif
}

void Renderer::DrawMesh(const std::string& meshName, const Mat4& world, const Material& material) {
    (void)material;
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_Pipeline || !m_Pipeline->IsReady()) return;

    Mat4 mvp = m_ViewProjection * world;
    auto* cmd = dx12->GetCommandList();
    m_Pipeline->Bind(cmd);
    m_Pipeline->SetMVP(mvp);
    m_Pipeline->DrawMesh(cmd, meshName);
#else
    (void)meshName; (void)world;
#endif
}

void Renderer::DrawMesh(const Mesh& mesh, const Mat4& transform, const Material& material) {
    // Legacy: use anonymous cache key
    const std::string key = "__anon";
    UploadMesh(key, mesh);
    DrawMesh(key, transform, material);
}

} // namespace Muk
