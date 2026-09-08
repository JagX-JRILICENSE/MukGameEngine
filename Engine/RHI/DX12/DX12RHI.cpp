#include "DX12RHI.h"
#include "Core/Log.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

namespace Muk {

DX12RHI::~DX12RHI() {
    Shutdown();
}

bool DX12RHI::Initialize(void* nativeWindowHandle, u32 width, u32 height) {
    m_Width = width;
    m_Height = height;

    if (!CreateDevice()) { MUK_CORE_ERROR("DX12: Failed to create device"); return false; }
    if (!CreateCommandObjects()) { MUK_CORE_ERROR("DX12: Failed to create command objects"); return false; }
    if (!CreateSwapchain(static_cast<HWND>(nativeWindowHandle))) { MUK_CORE_ERROR("DX12: Failed to create swapchain"); return false; }
    if (!CreateRTVs()) { MUK_CORE_ERROR("DX12: Failed to create RTVs"); return false; }
    if (!CreateDepthBuffer()) { MUK_CORE_ERROR("DX12: Failed to create depth buffer"); return false; }
    if (!CreateImGuiSrvHeap()) { MUK_CORE_ERROR("DX12: Failed to create ImGui SRV heap"); return false; }

    m_Initialized = true;
    MUK_CORE_INFO("DX12 RHI initialized ({0}x{1}) with depth + SRV heap", width, height);
    return true;
}

void DX12RHI::Shutdown() {
    if (!m_Initialized) return;
    WaitForGPU();

    if (m_FenceEvent) {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }

    for (u32 i = 0; i < FrameCount; ++i) {
        m_RenderTargets[i].Reset();
        m_CommandAllocators[i].Reset();
    }

    m_DepthStencil.Reset();
    m_DSVHeap.Reset();
    m_ImGuiSrvHeap.Reset();
    m_RTVHeap.Reset();
    m_Swapchain.Reset();
    m_CommandList.Reset();
    m_CommandQueue.Reset();
    m_Fence.Reset();
    m_Device.Reset();
    m_Factory.Reset();

    m_Initialized = false;
}

bool DX12RHI::CreateDevice() {
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory))))
        return false;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; m_Factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device))))
            break;
        adapter.Reset();
    }

    if (!m_Device) {
        ComPtr<IDXGIAdapter> warpAdapter;
        m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        if (FAILED(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device))))
            return false;
        MUK_CORE_WARN("DX12: Using WARP software adapter");
    }
    return true;
}

bool DX12RHI::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue))))
        return false;

    for (u32 i = 0; i < FrameCount; ++i) {
        if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i]))))
            return false;
    }

    if (FAILED(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_CommandList))))
        return false;
    m_CommandList->Close();

    if (FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence))))
        return false;

    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_FenceEvent) return false;
    m_FenceValues[0] = 1;
    return true;
}

bool DX12RHI::CreateSwapchain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_Width;
    swapChainDesc.Height = m_Height;
    swapChainDesc.Format = m_BackBufferFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    if (FAILED(m_Factory->CreateSwapChainForHwnd(m_CommandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain)))
        return false;

    m_Factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(swapChain.As(&m_Swapchain))) return false;
    m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();
    return true;
}

bool DX12RHI::CreateRTVs() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    if (FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap))))
        return false;

    m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
    for (u32 i = 0; i < FrameCount; ++i) {
        if (FAILED(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i]))))
            return false;
        m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_RTVDescriptorSize;
    }
    return true;
}

bool DX12RHI::CreateDepthBuffer() {
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    if (FAILED(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap))))
        return false;

    m_DSVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = m_Width;
    depthDesc.Height = m_Height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = m_DepthFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = m_DepthFormat;
    clearValue.DepthStencil.Depth = 1.0f;

    if (FAILED(m_Device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
            IID_PPV_ARGS(&m_DepthStencil))))
        return false;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = m_DepthFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    m_Device->CreateDepthStencilView(m_DepthStencil.Get(), &dsvDesc,
                                     m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
    return true;
}

bool DX12RHI::CreateImGuiSrvHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = ImGuiSrvCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_ImGuiSrvHeap))))
        return false;

    m_SrvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_ImGuiSrvNext = 1;
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RHI::GetCurrentRTV() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += m_FrameIndex * m_RTVDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RHI::GetDSV() const {
    return m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RHI::AllocImGuiSrv(D3D12_GPU_DESCRIPTOR_HANDLE* outGpu) {
    D3D12_CPU_DESCRIPTOR_HANDLE cpu = m_ImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpu = m_ImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart();

    if (m_ImGuiSrvNext >= ImGuiSrvCount)
        m_ImGuiSrvNext = 1;

    cpu.ptr += m_ImGuiSrvNext * m_SrvDescriptorSize;
    gpu.ptr += m_ImGuiSrvNext * m_SrvDescriptorSize;
    m_ImGuiSrvNext++;

    if (outGpu) *outGpu = gpu;
    return cpu;
}

void DX12RHI::FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE) {}

void DX12RHI::BindSwapchainTargets() {
    auto rtv = GetCurrentRTV();
    auto dsv = GetDSV();
    m_CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_Width);
    viewport.Height = static_cast<float>(m_Height);
    viewport.MaxDepth = 1.0f;
    m_CommandList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
    m_CommandList->RSSetScissorRects(1, &scissor);
}

void DX12RHI::BeginFrame() {
    const UINT64 currentFenceValue = m_FenceValues[m_FrameIndex];
    if (m_Fence->GetCompletedValue() < currentFenceValue) {
        m_Fence->SetEventOnCompletion(currentFenceValue, m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    m_CommandAllocators[m_FrameIndex]->Reset();
    m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_RenderTargets[m_FrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_CommandList->ResourceBarrier(1, &barrier);

    BindSwapchainTargets();

    const float clearColor[] = { 0.05f, 0.05f, 0.07f, 1.0f };
    m_CommandList->ClearRenderTargetView(GetCurrentRTV(), clearColor, 0, nullptr);
    m_CommandList->ClearDepthStencilView(GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    if (m_ImGuiSrvHeap) {
        ID3D12DescriptorHeap* heaps[] = { m_ImGuiSrvHeap.Get() };
        m_CommandList->SetDescriptorHeaps(1, heaps);
    }
}

void DX12RHI::EndFrame() {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_RenderTargets[m_FrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_CommandList->ResourceBarrier(1, &barrier);

    m_CommandList->Close();

    ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(1, cmdLists);
    m_Swapchain->Present(1, 0);
    MoveToNextFrame();
}

void DX12RHI::MoveToNextFrame() {
    const UINT64 currentFenceValue = m_FenceValues[m_FrameIndex];
    m_CommandQueue->Signal(m_Fence.Get(), currentFenceValue);
    m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();

    if (m_Fence->GetCompletedValue() < m_FenceValues[m_FrameIndex]) {
        m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
    m_FenceValues[m_FrameIndex] = currentFenceValue + 1;
}

void DX12RHI::WaitForGPU() {
    const UINT64 fenceValue = m_FenceValues[m_FrameIndex];
    m_CommandQueue->Signal(m_Fence.Get(), fenceValue);
    m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
    WaitForSingleObject(m_FenceEvent, INFINITE);
    m_FenceValues[m_FrameIndex]++;
}

void DX12RHI::Resize(u32 width, u32 height) {
    if (width == 0 || height == 0 || (width == m_Width && height == m_Height))
        return;

    WaitForGPU();

    for (u32 i = 0; i < FrameCount; ++i) {
        m_RenderTargets[i].Reset();
        m_FenceValues[i] = m_FenceValues[m_FrameIndex];
    }
    m_DepthStencil.Reset();

    m_Width = width;
    m_Height = height;

    DXGI_SWAP_CHAIN_DESC desc = {};
    m_Swapchain->GetDesc(&desc);
    m_Swapchain->ResizeBuffers(FrameCount, width, height, desc.BufferDesc.Format, desc.Flags);
    m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();
    CreateRTVs();
    CreateDepthBuffer();
}

} // namespace Muk
