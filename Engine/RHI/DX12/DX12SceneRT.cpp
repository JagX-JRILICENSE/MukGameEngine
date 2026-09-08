#include "DX12SceneRT.h"
#include "Core/Log.h"

namespace Muk {

bool DX12SceneRT::Create(ID3D12Device* device, u32 width, u32 height,
                         DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat,
                         ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax) {
    if (!device || width == 0 || height == 0) return false;
    Destroy();

    m_Width = width;
    m_Height = height;

    // Color (RENDER_TARGET + SRV)
    D3D12_RESOURCE_DESC colorDesc = {};
    colorDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.DepthOrArraySize = 1;
    colorDesc.MipLevels = 1;
    colorDesc.Format = colorFormat;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE colorClear = {};
    colorClear.Format = colorFormat;
    colorClear.Color[0] = 0.08f; colorClear.Color[1] = 0.08f;
    colorClear.Color[2] = 0.12f; colorClear.Color[3] = 1.0f;

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &colorDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &colorClear, IID_PPV_ARGS(&m_Color))))
        return false;

    // Depth
    D3D12_RESOURCE_DESC depthDesc = colorDesc;
    depthDesc.Format = depthFormat;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClear = {};
    depthClear.Format = depthFormat;
    depthClear.DepthStencil.Depth = 1.0f;

    if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear, IID_PPV_ARGS(&m_Depth))))
        return false;

    // RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap))))
        return false;
    m_RtvCpu = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(m_Color.Get(), nullptr, m_RtvCpu);

    // DSV heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap))))
        return false;
    m_DsvCpu = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(m_Depth.Get(), &dsvDesc, m_DsvCpu);

    // SRV on shared heap for ImGui
    if (srvHeap && srvNext && *srvNext < srvMax) {
        m_ColorSrvCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
        m_ColorSrvGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_ColorSrvCpu.ptr += (*srvNext) * srvSize;
        m_ColorSrvGpu.ptr += (*srvNext) * srvSize;
        (*srvNext)++;

        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.Format = colorFormat;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_Color.Get(), &srv, m_ColorSrvCpu);
    }

    m_Valid = true;
    m_InPixelShaderState = false;
    MUK_CORE_INFO("SceneRT created {0}x{1}", width, height);
    return true;
}

void DX12SceneRT::Destroy() {
    m_Color.Reset();
    m_Depth.Reset();
    m_RtvHeap.Reset();
    m_DsvHeap.Reset();
    m_Valid = false;
    m_Width = m_Height = 0;
}

bool DX12SceneRT::Resize(ID3D12Device* device, u32 width, u32 height,
                         DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat,
                         ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax) {
    if (width == m_Width && height == m_Height && m_Valid) return true;
    return Create(device, width, height, colorFormat, depthFormat, srvHeap, srvSize, srvNext, srvMax);
}

void DX12SceneRT::Begin(ID3D12GraphicsCommandList* cmd, const float clearColor[4]) {
    if (!m_Valid || !cmd) return;

    if (m_InPixelShaderState) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_Color.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &barrier);
        m_InPixelShaderState = false;
    }

    cmd->OMSetRenderTargets(1, &m_RtvCpu, FALSE, &m_DsvCpu);
    cmd->ClearRenderTargetView(m_RtvCpu, clearColor, 0, nullptr);
    cmd->ClearDepthStencilView(m_DsvCpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width = (float)m_Width;
    vp.Height = (float)m_Height;
    vp.MaxDepth = 1.0f;
    cmd->RSSetViewports(1, &vp);

    D3D12_RECT sc = { 0, 0, (LONG)m_Width, (LONG)m_Height };
    cmd->RSSetScissorRects(1, &sc);
}

void DX12SceneRT::End(ID3D12GraphicsCommandList* cmd) {
    if (!m_Valid || !cmd) return;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_Color.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &barrier);
    m_InPixelShaderState = true;
}

} // namespace Muk
