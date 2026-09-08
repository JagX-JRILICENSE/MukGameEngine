#pragma once

#include "RHI/RHI.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>

namespace Muk {

using Microsoft::WRL::ComPtr;

/**
 * DirectX 12 RHI implementation
 * Provides device, command queues, swapchain, fences, and basic frame management.
 */
class DX12RHI : public RHI {
public:
    DX12RHI() = default;
    ~DX12RHI() override;

    bool Initialize(void* nativeWindowHandle, u32 width, u32 height) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Resize(u32 width, u32 height) override;

    // Accessors for higher-level renderer
    ID3D12Device* GetDevice() const { return m_Device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandList.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
    DXGI_FORMAT GetBackBufferFormat() const { return m_BackBufferFormat; }

    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }

private:
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapchain(HWND hwnd);
    bool CreateRTVs();
    void WaitForGPU();
    void MoveToNextFrame();

    static constexpr u32 FrameCount = 2;

    ComPtr<ID3D12Device> m_Device;
    ComPtr<IDXGIFactory4> m_Factory;
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_CommandList;
    ComPtr<IDXGISwapChain3> m_Swapchain;
    ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
    ComPtr<ID3D12Resource> m_RenderTargets[FrameCount];
    ComPtr<ID3D12Fence> m_Fence;

    HANDLE m_FenceEvent = nullptr;
    UINT64 m_FenceValues[FrameCount] = {};
    u32 m_FrameIndex = 0;
    u32 m_RTVDescriptorSize = 0;

    u32 m_Width = 0;
    u32 m_Height = 0;
    DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    bool m_Initialized = false;
};

} // namespace Muk
