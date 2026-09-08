#include "DX12ShadowMap.h"
#include "Core/Log.h"
#include <cmath>

namespace Muk {

bool DX12ShadowMap::Create(ID3D12Device* device, u32 resolution,
                           ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax) {
    if (!device || resolution < 64) return false;
    Destroy();
    m_Resolution = resolution;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = resolution;
    desc.Height = resolution;
    desc.DepthOrArraySize = kShadowCascades;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = DXGI_FORMAT_D32_FLOAT;
    clear.DepthStencil.Depth = 1.0f;

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&m_DepthArray))))
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = kShadowCascades;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap))))
        return false;

    u32 dsvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    D3D12_CPU_DESCRIPTOR_HANDLE base = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();

    for (u32 i = 0; i < kShadowCascades; ++i) {
        m_DsvCpu[i] = base;
        m_DsvCpu[i].ptr += i * dsvSize;

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        dsv.Format = DXGI_FORMAT_D32_FLOAT;
        dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsv.Texture2DArray.ArraySize = 1;
        dsv.Texture2DArray.FirstArraySlice = i;
        dsv.Texture2DArray.MipSlice = 0;
        device->CreateDepthStencilView(m_DepthArray.Get(), &dsv, m_DsvCpu[i]);
        m_LightVP[i] = Mat4::Identity();
    }

    if (srvHeap && srvNext && *srvNext < srvMax) {
        m_SrvCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
        m_SrvGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_SrvCpu.ptr += (*srvNext) * srvSize;
        m_SrvGpu.ptr += (*srvNext) * srvSize;
        (*srvNext)++;

        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.Format = DXGI_FORMAT_R32_FLOAT;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2DArray.MipLevels = 1;
        srv.Texture2DArray.ArraySize = kShadowCascades;
        srv.Texture2DArray.FirstArraySlice = 0;
        device->CreateShaderResourceView(m_DepthArray.Get(), &srv, m_SrvCpu);
    }

    m_Valid = true;
    m_InShaderReadable = false;
    MUK_CORE_INFO("Cascaded ShadowMap {0}x{0} x{1} ready", resolution, kShadowCascades);
    return true;
}

void DX12ShadowMap::Destroy() {
    m_DepthArray.Reset();
    m_DsvHeap.Reset();
    m_Valid = false;
}

void DX12ShadowMap::UpdateCascades(const Vec3& lightDir, const Vec3& focus, f32 maxRadius) {
    f32 len = std::sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
    if (len < 1e-5f) len = 1.0f;
    Vec3 dir = { lightDir.x / len, lightDir.y / len, lightDir.z / len };

    // Split distances (world units from focus)
    m_Splits[0] = maxRadius * 0.2f;
    m_Splits[1] = maxRadius * 0.5f;
    m_Splits[2] = maxRadius;

    for (u32 i = 0; i < kShadowCascades; ++i) {
        f32 radius = m_Splits[i];
        Vec3 eye = {
            focus.x - dir.x * (radius * 1.5f),
            focus.y - dir.y * (radius * 1.5f),
            focus.z - dir.z * (radius * 1.5f)
        };
        Vec3 up = { 0, 1, 0 };
        if (std::fabs(Vec3::Dot(dir, up)) > 0.95f) up = { 0, 0, 1 };

        Mat4 view = Mat4::LookAt(eye, focus, up);

        f32 nearZ = 0.5f;
        f32 farZ = radius * 3.0f;
        f32 r = radius;
        Mat4 proj{};
        proj.m[0] = 1.0f / r;
        proj.m[5] = 1.0f / r;
        proj.m[10] = 1.0f / (nearZ - farZ);
        proj.m[14] = nearZ / (nearZ - farZ);
        proj.m[15] = 1.0f;

        m_LightVP[i] = proj * view;
    }
}

Mat4 DX12ShadowMap::GetLightViewProj(u32 cascade) const {
    return cascade < kShadowCascades ? m_LightVP[cascade] : Mat4::Identity();
}

void DX12ShadowMap::BeginCascade(ID3D12GraphicsCommandList* cmd, u32 cascadeIndex) {
    if (!m_Valid || !cmd || cascadeIndex >= kShadowCascades) return;

    if (m_InShaderReadable) {
        D3D12_RESOURCE_BARRIER b = {};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = m_DepthArray.Get();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &b);
        m_InShaderReadable = false;
    }

    cmd->OMSetRenderTargets(0, nullptr, FALSE, &m_DsvCpu[cascadeIndex]);
    cmd->ClearDepthStencilView(m_DsvCpu[cascadeIndex], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width = (float)m_Resolution;
    vp.Height = (float)m_Resolution;
    vp.MaxDepth = 1.0f;
    cmd->RSSetViewports(1, &vp);
    D3D12_RECT sc = { 0, 0, (LONG)m_Resolution, (LONG)m_Resolution };
    cmd->RSSetScissorRects(1, &sc);
}

void DX12ShadowMap::EndAll(ID3D12GraphicsCommandList* cmd) {
    if (!m_Valid || !cmd) return;
    D3D12_RESOURCE_BARRIER b = {};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = m_DepthArray.Get();
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &b);
    m_InShaderReadable = true;
}

} // namespace Muk
