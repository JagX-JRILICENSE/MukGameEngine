#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Core/Core.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

/**
 * Single-cascade directional shadow map.
 * Depth-only render target + shader-visible SRV for sampling in lit pass.
 */
class DX12ShadowMap {
public:
    bool Create(ID3D12Device* device, u32 resolution,
                ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax);
    void Destroy();

    void Begin(ID3D12GraphicsCommandList* cmd);
    void End(ID3D12GraphicsCommandList* cmd);

    // Light-space VP looking along -lightDir toward scene center
    void UpdateLightMatrix(const Vec3& lightDir, const Vec3& focus, f32 radius, f32 nearZ = 0.5f, f32 farZ = 80.0f);

    Mat4 GetLightViewProj() const { return m_LightVP; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSrvGpu() const { return m_SrvGpu; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_DsvCpu; }
    u32 GetResolution() const { return m_Resolution; }
    bool IsValid() const { return m_Valid; }
    ID3D12Resource* GetResource() const { return m_Depth.Get(); }

private:
    ComPtr<ID3D12Resource> m_Depth;
    ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_DsvCpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_SrvCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_SrvGpu = {};
    Mat4 m_LightVP = Mat4::Identity();
    u32 m_Resolution = 1024;
    bool m_Valid = false;
    bool m_InShaderReadable = false;
};

} // namespace Muk
