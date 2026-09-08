#include "DX12Texture.h"
#include "Core/Log.h"
#include <cstring>
#include <vector>

namespace Muk {

void DX12TextureCache::Initialize(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap,
                                  u32 srvDescriptorSize, u32* srvBumpIndex, u32 srvMaxCount) {
    m_Device = device;
    m_SrvHeap = srvHeap;
    m_SrvSize = srvDescriptorSize;
    m_SrvNext = srvBumpIndex;
    m_SrvMax = srvMaxCount;
    CreateWhiteTexture(device);
}

void DX12TextureCache::Shutdown() {
    m_Cache.clear();
    m_WhiteResource.Reset();
    m_Device = nullptr;
    m_SrvHeap = nullptr;
}

bool DX12TextureCache::CreateWhiteTexture(ID3D12Device* device) {
    if (!device || !m_SrvHeap || !m_SrvNext) return false;

    D3D12_HEAP_PROPERTIES uploadProps = {};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    // Simple 1x1 RGBA white in an upload buffer interpreted as a buffer - use committed texture via upload
    const u32 pixel = 0xFFFFFFFFu;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = 1;
    texDesc.Height = 1;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Use UPLOAD heap for simplicity (small textures / white)
    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;

    // For textures on UPLOAD we still need row pitch layout - use buffer staging approach:
    // Create DEFAULT texture + UPLOAD buffer copy would need command list.
    // For white 1x1 use a committed resource on UPLOAD with placed footprint via GetCopyableFootprints path.
    // Simpler: create DEFAULT and write via Map on a staging buffer with immediate queue.
    // Minimal path: create texture on DEFAULT empty and SRV; pixel stays black unless we copy.
    // We'll create upload buffer and use a one-shot command list if needed.
    // For reliability without extra queue: store as buffer SRV is wrong.
    // Use CreateCommittedResource UPLOAD as a buffer of 256 bytes and document white via root constants instead.
    // Practical approach for 1x1: DEFAULT texture + Map not allowed. Use UpdateSubresources pattern offline.

    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_WhiteResource))))
        return false;

    // Staging upload
    UINT64 uploadSize = 0;
    device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadSize);

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = uploadSize < 256 ? 256 : uploadSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES upHeap = {};
    upHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    ComPtr<ID3D12Resource> staging;
    if (FAILED(device->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&staging))))
        return false;

    void* mapped = nullptr;
    staging->Map(0, nullptr, &mapped);
    std::memset(mapped, 0xFF, 4);
    staging->Unmap(0, nullptr);

    // One-shot copy
    ComPtr<ID3D12CommandAllocator> alloc;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12Fence> fence;

    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(), nullptr, IID_PPV_ARGS(&list));
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    device->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue));
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = m_WhiteResource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT numRows = 0;
    UINT64 rowSize = 0, total = 0;
    device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSize, &total);

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = staging.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = footprint;

    list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_WhiteResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    // Resource starts COMMON; set COPY_DEST first
    D3D12_RESOURCE_BARRIER toCopy = barrier;
    toCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1, &toCopy);
    list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    list->ResourceBarrier(1, &barrier);
    list->Close();

    ID3D12CommandList* lists[] = { list.Get() };
    queue->ExecuteCommandLists(1, lists);
    queue->Signal(fence.Get(), 1);
    HANDLE ev = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fence->SetEventOnCompletion(1, ev);
    WaitForSingleObject(ev, INFINITE);
    CloseHandle(ev);

    // SRV
    if (*m_SrvNext >= m_SrvMax) *m_SrvNext = 1;
    m_WhiteCpu = m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
    m_WhiteGpu = m_SrvHeap->GetGPUDescriptorHandleForHeapStart();
    m_WhiteCpu.ptr += (*m_SrvNext) * m_SrvSize;
    m_WhiteGpu.ptr += (*m_SrvNext) * m_SrvSize;
    (*m_SrvNext)++;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_WhiteResource.Get(), &srv, m_WhiteCpu);

    MUK_CORE_INFO("DX12TextureCache: white 1x1 fallback ready");
    return true;
}

bool DX12TextureCache::Has(const std::string& name) const {
    return m_Cache.find(name) != m_Cache.end();
}

GPUTexture* DX12TextureCache::Get(const std::string& name) {
    auto it = m_Cache.find(name);
    return it != m_Cache.end() ? it->second.get() : nullptr;
}

bool DX12TextureCache::Upload(ID3D12Device* device, ID3D12GraphicsCommandList* /*cmdList*/,
                              ID3D12CommandQueue* /*queue*/, const std::string& name,
                              const Texture& cpuTex) {
    if (!cpuTex.IsValid() || !device || !m_SrvHeap) return false;
    if (Has(name)) return true;

    auto gpu = std::make_unique<GPUTexture>();
    gpu->Width = cpuTex.Width;
    gpu->Height = cpuTex.Height;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = cpuTex.Width;
    texDesc.Height = cpuTex.Height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
    if (FAILED(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&gpu->Resource))))
        return false;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT numRows = 0;
    UINT64 rowSize = 0, total = 0;
    device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSize, &total);

    D3D12_HEAP_PROPERTIES upHeap = {};
    upHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = total;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> staging;
    if (FAILED(device->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&staging))))
        return false;

    void* mapped = nullptr;
    staging->Map(0, nullptr, &mapped);
    u8* dst = reinterpret_cast<u8*>(mapped) + footprint.Offset;
    const u32 srcPitch = cpuTex.Width * 4;
    for (u32 y = 0; y < cpuTex.Height; ++y) {
        std::memcpy(dst + y * footprint.Footprint.RowPitch,
                    cpuTex.Pixels.data() + y * srcPitch, srcPitch);
    }
    staging->Unmap(0, nullptr);

    // One-shot copy command list
    ComPtr<ID3D12CommandAllocator> alloc;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12CommandQueue> q;
    ComPtr<ID3D12Fence> fence;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(), nullptr, IID_PPV_ARGS(&list));
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    device->CreateCommandQueue(&qd, IID_PPV_ARGS(&q));
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    D3D12_RESOURCE_BARRIER toCopy = {};
    toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toCopy.Transition.pResource = gpu->Resource.Get();
    toCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    list->ResourceBarrier(1, &toCopy);

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = gpu->Resource.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = staging.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint = footprint;

    list->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    D3D12_RESOURCE_BARRIER toSrv = toCopy;
    toSrv.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    toSrv.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    list->ResourceBarrier(1, &toSrv);
    list->Close();

    ID3D12CommandList* lists[] = { list.Get() };
    q->ExecuteCommandLists(1, lists);
    q->Signal(fence.Get(), 1);
    HANDLE ev = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fence->SetEventOnCompletion(1, ev);
    WaitForSingleObject(ev, INFINITE);
    CloseHandle(ev);

    if (*m_SrvNext >= m_SrvMax) *m_SrvNext = 1;
    gpu->CpuSrv = m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
    gpu->GpuSrv = m_SrvHeap->GetGPUDescriptorHandleForHeapStart();
    gpu->CpuSrv.ptr += (*m_SrvNext) * m_SrvSize;
    gpu->GpuSrv.ptr += (*m_SrvNext) * m_SrvSize;
    (*m_SrvNext)++;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(gpu->Resource.Get(), &srv, gpu->CpuSrv);

    gpu->Valid = true;
    m_Cache[name] = std::move(gpu);
    MUK_CORE_INFO("GPU texture uploaded: {0} ({1}x{2})", name.c_str(), (int)cpuTex.Width, (int)cpuTex.Height);
    return true;
}

} // namespace Muk
