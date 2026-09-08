#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Core/Core.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

/**
 * Offscreen color + depth target for editor viewport (render-to-texture).
 * Color SRV is displayed inside ImGui::Image.
 */
class DX12SceneRT {
public:
    bool Create(ID3D12Device* device, u32 width, u32 height,
                DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat,
                ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax);
    void Destroy();
    bool Resize(ID3D12Device* device, u32 width, u32 height,
                DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat,
                ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax);

    void Begin(ID3D12GraphicsCommandList* cmd, const float clearColor[4]);
    void End(ID3D12GraphicsCommandList* cmd); // transition color to PIXEL_SHADER_RESOURCE

    D3D12_GPU_DESCRIPTOR_HANDLE GetColorSrvGpu() const { return m_ColorSrvGpu; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return m_RtvCpu; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_DsvCpu; }
    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    bool IsValid() const { return m_Valid; }
    ID3D12Resource* GetColorResource() const { return m_Color.Get(); }

private:
    ComPtr<ID3D12Resource> m_Color;
    ComPtr<ID3D12Resource> m_Depth;
    ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_DsvHeap;

    D3D12_CPU_DESCRIPTOR_HANDLE m_RtvCpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_DsvCpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_ColorSrvCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ColorSrvGpu = {};

    u32 m_Width = 0;
    u32 m_Height = 0;
    bool m_Valid = false;
    bool m_InPixelShaderState = false;
};

} // namespace Muk
