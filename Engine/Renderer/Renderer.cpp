#include "Renderer.h"
#include "Core/Log.h"
#include "Math/Math.h"
#include "RHI/DX12/DX12RHI.h"
#include "RHI/DX12/DX12Pipeline.h"
#include "RHI/DX12/DX12SceneRT.h"

namespace Muk {

Renderer::~Renderer() { Shutdown(); }

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
        if (!m_Pipeline->Initialize(dx12->GetDevice(), dx12->GetBackBufferFormat(), dx12->GetDepthFormat(),
                                    dx12->GetImGuiSrvHeap(), dx12->GetSrvDescriptorSize(),
                                    dx12->GetSrvBumpIndex(), dx12->GetSrvMaxCount())) {
            MUK_CORE_ERROR("Failed to initialize DX12Pipeline");
            m_Pipeline.reset();
        }
        m_SceneRT = std::make_unique<DX12SceneRT>();
    }
#endif

    RebuildViewProjection();
    m_Initialized = true;
    MUK_CORE_INFO("Renderer ready (textures + SceneRT)");
    return true;
}

void Renderer::Shutdown() {
    if (m_SceneRT) { m_SceneRT->Destroy(); m_SceneRT.reset(); }
    if (m_Pipeline) { m_Pipeline->Shutdown(); m_Pipeline.reset(); }
    if (m_RHI) { m_RHI->Shutdown(); m_RHI.reset(); }
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
    f32 aspect = m_Height > 0 ? (f32)m_Width / (f32)m_Height : 1.0f;
    if (m_RenderingToSceneRT && m_SceneRT && m_SceneRT->IsValid())
        aspect = (f32)m_SceneRT->GetWidth() / (f32)m_SceneRT->GetHeight();
    Mat4 view = Mat4::LookAt(m_Camera.Eye, m_Camera.Target, m_Camera.Up);
    Mat4 proj = Mat4::Perspective(ToRadians(m_Camera.FOVDegrees), aspect, m_Camera.Near, m_Camera.Far);
    m_ViewProjection = proj * view;
}

bool Renderer::UploadMesh(const std::string& name, const Mesh& mesh) {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_Pipeline) return false;
    return m_Pipeline->UploadMesh(dx12->GetDevice(), name, mesh);
#else
    (void)name; (void)mesh; return false;
#endif
}

bool Renderer::UploadTexture(const std::string& name, const Texture& texture) {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_Pipeline) return false;
    return m_Pipeline->UploadTexture(dx12->GetDevice(), name, texture);
#else
    (void)name; (void)texture; return false;
#endif
}

void Renderer::DrawMesh(const std::string& meshName, const Mat4& world, const Material& material) {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_Pipeline || !m_Pipeline->IsReady()) return;

    // Ensure albedo on GPU
    if (material.AlbedoMap && material.AlbedoMap->IsValid()) {
        std::string key = material.AlbedoTexture.empty() ? material.AlbedoMap->Name : material.AlbedoTexture;
        if (!key.empty())
            m_Pipeline->UploadTexture(dx12->GetDevice(), key, *material.AlbedoMap);
    }

    Mat4 mvp = m_ViewProjection * world;
    auto* cmd = dx12->GetCommandList();
    m_Pipeline->Bind(cmd);
    m_Pipeline->SetMaterialParams(mvp, material);

    // Bind albedo SRV table
    D3D12_GPU_DESCRIPTOR_HANDLE srv = m_Pipeline->Textures().GetWhiteSrv();
    if (material.AlbedoMap) {
        std::string key = material.AlbedoTexture.empty() ? material.AlbedoMap->Name : material.AlbedoTexture;
        if (auto* gpu = m_Pipeline->Textures().Get(key))
            srv = gpu->GpuSrv;
    }
    cmd->SetGraphicsRootDescriptorTable(1, srv);

    m_Pipeline->DrawMesh(cmd, meshName);
#else
    (void)meshName; (void)world; (void)material;
#endif
}

void Renderer::DrawMesh(const Mesh& mesh, const Mat4& transform, const Material& material) {
    UploadMesh("__anon", mesh);
    DrawMesh("__anon", transform, material);
}

bool Renderer::EnsureSceneRT(u32 width, u32 height) {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_SceneRT) return false;
    if (width < 8) width = 8;
    if (height < 8) height = 8;
    return m_SceneRT->Resize(dx12->GetDevice(), width, height,
                             dx12->GetBackBufferFormat(), dx12->GetDepthFormat(),
                             dx12->GetImGuiSrvHeap(), dx12->GetSrvDescriptorSize(),
                             dx12->GetSrvBumpIndex(), dx12->GetSrvMaxCount());
#else
    (void)width; (void)height; return false;
#endif
}

void Renderer::BeginSceneRT() {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_SceneRT || !m_SceneRT->IsValid()) return;
    const float clear[4] = { 0.1f, 0.12f, 0.18f, 1.0f };
    m_SceneRT->Begin(dx12->GetCommandList(), clear);
    m_RenderingToSceneRT = true;
    RebuildViewProjection();
#endif
}

void Renderer::EndSceneRT() {
#ifdef MUK_RHI_DX12
    auto* dx12 = dynamic_cast<DX12RHI*>(m_RHI.get());
    if (!dx12 || !m_SceneRT) return;
    m_SceneRT->End(dx12->GetCommandList());
    m_RenderingToSceneRT = false;
    dx12->BindSwapchainTargets();
    RebuildViewProjection();
#endif
}

void* Renderer::GetSceneRTGpuHandle() const {
#ifdef MUK_RHI_DX12
    if (m_SceneRT && m_SceneRT->IsValid()) {
        // ImGui expects ImTextureID = GPU handle ptr value on DX12
        auto h = m_SceneRT->GetColorSrvGpu();
        return (void*)h.ptr;
    }
#endif
    return nullptr;
}

u32 Renderer::GetSceneRTWidth() const {
    return m_SceneRT && m_SceneRT->IsValid() ? m_SceneRT->GetWidth() : 0;
}

u32 Renderer::GetSceneRTHeight() const {
    return m_SceneRT && m_SceneRT->IsValid() ? m_SceneRT->GetHeight() : 0;
}

} // namespace Muk
