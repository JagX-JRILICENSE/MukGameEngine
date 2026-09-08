#pragma once

#include "RHI/RHI.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>

namespace Muk {

using Microsoft::WRL::ComPtr;

class DX12RHI : public RHI {
public:
    DX12RHI() = default;
    ~DX12RHI() override;

    bool Initialize(void* nativeWindowHandle, u32 width, u32 height) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Resize(u32 width, u32 height) override;

    ID3D12Device* GetDevice() const { return m_Device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandList.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
    DXGI_FORMAT GetBackBufferFormat() const { return m_BackBufferFormat; }
    DXGI_FORMAT GetDepthFormat() const { return m_DepthFormat; }

    // ImGui font / texture SRV heap (shader-visible)
    ID3D12DescriptorHeap* GetImGuiSrvHeap() const { return m_ImGuiSrvHeap.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE AllocImGuiSrv(D3D12_GPU_DESCRIPTOR_HANDLE* outGpu = nullptr);
    void FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpu); // simple bump allocator - no free for now

    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    u32 GetFrameIndex() const { return m_FrameIndex; }
    static constexpr u32 GetFrameCount() { return FrameCount; }

private:
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapchain(HWND hwnd);
    bool CreateRTVs();
    bool CreateDepthBuffer();
    bool CreateImGuiSrvHeap();
    void WaitForGPU();
    void MoveToNextFrame();

    static constexpr u32 FrameCount = 2;
    static constexpr u32 ImGuiSrvCount = 64;

    ComPtr<ID3D12Device> m_Device;
    ComPtr<IDXGIFactory4> m_Factory;
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_CommandList;
    ComPtr<IDXGISwapChain3> m_Swapchain;
    ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
    ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
    ComPtr<ID3D12DescriptorHeap> m_ImGuiSrvHeap;
    ComPtr<ID3D12Resource> m_RenderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_DepthStencil;
    ComPtr<ID3D12Fence> m_Fence;

    HANDLE m_FenceEvent = nullptr;
    UINT64 m_FenceValues[FrameCount] = {};
    u32 m_FrameIndex = 0;
    u32 m_RTVDescriptorSize = 0;
    u32 m_DSVDescriptorSize = 0;
    u32 m_SrvDescriptorSize = 0;
    u32 m_ImGuiSrvNext = 1; // 0 reserved for font

    u32 m_Width = 0;
    u32 m_Height = 0;
    DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT m_DepthFormat = DXGI_FORMAT_D32_FLOAT;
    bool m_Initialized = false;
};

} // namespace Muk
