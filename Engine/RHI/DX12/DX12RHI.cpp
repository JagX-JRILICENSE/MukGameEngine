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

    if (!CreateDevice()) {
        MUK_CORE_ERROR("DX12: Failed to create device");
        return false;
    }
    if (!CreateCommandObjects()) {
        MUK_CORE_ERROR("DX12: Failed to create command objects");
        return false;
    }
    if (!CreateSwapchain(static_cast<HWND>(nativeWindowHandle))) {
        MUK_CORE_ERROR("DX12: Failed to create swapchain");
        return false;
    }
    if (!CreateRTVs()) {
        MUK_CORE_ERROR("DX12: Failed to create RTVs");
        return false;
    }

    m_Initialized = true;
    MUK_CORE_INFO("DX12 RHI initialized ({0}x{1})", width, height);
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

    m_RTVHeap.Reset();
    m_Swapchain.Reset();
    m_CommandList.Reset();
    m_CommandQueue.Reset();
    m_Fence.Reset();
    m_Device.Reset();
    m_Factory.Reset();

    m_Initialized = false;
    MUK_CORE_INFO("DX12 RHI shut down");
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

    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)))) {
        return false;
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; m_Factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)))) {
            break;
        }
        adapter.Reset();
    }

    if (!m_Device) {
        // Fallback to WARP
        ComPtr<IDXGIAdapter> warpAdapter;
        m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        if (FAILED(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)))) {
            return false;
        }
        MUK_CORE_WARN("DX12: Using WARP software adapter");
    }

    return true;
}

bool DX12RHI::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)))) {
        return false;
    }

    for (u32 i = 0; i < FrameCount; ++i) {
        if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i])))) {
            return false;
        }
    }

    if (FAILED(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_CommandList)))) {
        return false;
    }

    // Command lists are created in recording state; close it for now
    m_CommandList->Close();

    if (FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)))) {
        return false;
    }

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
    if (FAILED(m_Factory->CreateSwapChainForHwnd(
            m_CommandQueue.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain))) {
        return false;
    }

    // Disable Alt+Enter fullscreen toggle (we handle it ourselves later)
    m_Factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (FAILED(swapChain.As(&m_Swapchain))) {
        return false;
    }

    m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();
    return true;
}

bool DX12RHI::CreateRTVs() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)))) {
        return false;
    }

    m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
    for (u32 i = 0; i < FrameCount; ++i) {
        if (FAILED(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i])))) {
            return false;
        }
        m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_RTVDescriptorSize;
    }

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12RHI::GetCurrentRTV() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += m_FrameIndex * m_RTVDescriptorSize;
    return handle;
}

void DX12RHI::BeginFrame() {
    // Wait for the previous frame using this allocator to finish
    const UINT64 currentFenceValue = m_FenceValues[m_FrameIndex];
    if (m_Fence->GetCompletedValue() < currentFenceValue) {
        m_Fence->SetEventOnCompletion(currentFenceValue, m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }

    m_CommandAllocators[m_FrameIndex]->Reset();
    m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr);

    // Transition back buffer to render target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_RenderTargets[m_FrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_CommandList->ResourceBarrier(1, &barrier);

    // Set render target and clear
    auto rtv = GetCurrentRTV();
    m_CommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    const float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f }; // Dark blue-gray
    m_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    // Viewport + scissor
    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_Width);
    viewport.Height = static_cast<float>(m_Height);
    viewport.MaxDepth = 1.0f;
    m_CommandList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
    m_CommandList->RSSetScissorRects(1, &scissor);
}

void DX12RHI::EndFrame() {
    // Transition back to present
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_RenderTargets[m_FrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_CommandList->ResourceBarrier(1, &barrier);

    m_CommandList->Close();

    ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(1, cmdLists);

    m_Swapchain->Present(1, 0); // VSync on

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

    m_Width = width;
    m_Height = height;

    DXGI_SWAP_CHAIN_DESC desc = {};
    m_Swapchain->GetDesc(&desc);
    m_Swapchain->ResizeBuffers(FrameCount, width, height, desc.BufferDesc.Format, desc.Flags);

    m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();
    CreateRTVs();

    MUK_CORE_INFO("DX12: Resized to {0}x{1}", width, height);
}

} // namespace Muk
