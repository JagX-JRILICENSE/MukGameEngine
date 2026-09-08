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
    desc.DepthOrArraySize = 1;
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
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&m_Depth))))
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap))))
        return false;

    m_DsvCpu = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(m_Depth.Get(), &dsv, m_DsvCpu);

    if (srvHeap && srvNext && *srvNext < srvMax) {
        m_SrvCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
        m_SrvGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_SrvCpu.ptr += (*srvNext) * srvSize;
        m_SrvGpu.ptr += (*srvNext) * srvSize;
        (*srvNext)++;

        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.Format = DXGI_FORMAT_R32_FLOAT;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_Depth.Get(), &srv, m_SrvCpu);
    }

    m_Valid = true;
    m_InShaderReadable = false;
    MUK_CORE_INFO("ShadowMap {0}x{0} ready", resolution);
    return true;
}

void DX12ShadowMap::Destroy() {
    m_Depth.Reset();
    m_DsvHeap.Reset();
    m_Valid = false;
}

void DX12ShadowMap::UpdateLightMatrix(const Vec3& lightDir, const Vec3& focus, f32 radius, f32 nearZ, f32 farZ) {
    f32 len = std::sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
    if (len < 1e-5f) len = 1.0f;
    Vec3 dir = { lightDir.x / len, lightDir.y / len, lightDir.z / len };

    // Place light camera opposite to direction from focus
    Vec3 eye = {
        focus.x - dir.x * (radius * 1.5f),
        focus.y - dir.y * (radius * 1.5f),
        focus.z - dir.z * (radius * 1.5f)
    };

    Vec3 up = { 0, 1, 0 };
    if (std::fabs(Vec3::Dot(dir, up)) > 0.95f)
        up = { 0, 0, 1 };

    Mat4 view = Mat4::LookAt(eye, focus, up);

    // Orthographic projection covering the scene sphere
    f32 r = radius;
    Mat4 proj{};
    // Column-major ortho RH: left right bottom top near far
    f32 l = -r, rt = r, b = -r, t = r;
    proj.m[0] = 2.0f / (rt - l);
    proj.m[5] = 2.0f / (t - b);
    proj.m[10] = 1.0f / (nearZ - farZ);
    proj.m[12] = -(rt + l) / (rt - l);
    proj.m[13] = -(t + b) / (t - b);
    proj.m[14] = nearZ / (nearZ - farZ);
    proj.m[15] = 1.0f;

    m_LightVP = proj * view;
}

void DX12ShadowMap::Begin(ID3D12GraphicsCommandList* cmd) {
    if (!m_Valid || !cmd) return;

    if (m_InShaderReadable) {
        D3D12_RESOURCE_BARRIER b = {};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = m_Depth.Get();
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &b);
        m_InShaderReadable = false;
    }

    cmd->OMSetRenderTargets(0, nullptr, FALSE, &m_DsvCpu);
    cmd->ClearDepthStencilView(m_DsvCpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width = (float)m_Resolution;
    vp.Height = (float)m_Resolution;
    vp.MaxDepth = 1.0f;
    cmd->RSSetViewports(1, &vp);

    D3D12_RECT sc = { 0, 0, (LONG)m_Resolution, (LONG)m_Resolution };
    cmd->RSSetScissorRects(1, &sc);
}

void DX12ShadowMap::End(ID3D12GraphicsCommandList* cmd) {
    if (!m_Valid || !cmd) return;
    D3D12_RESOURCE_BARRIER b = {};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = m_Depth.Get();
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    b.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &b);
    m_InShaderReadable = true;
}

} // namespace Muk
