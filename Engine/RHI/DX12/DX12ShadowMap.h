#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Core/Core.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

static constexpr u32 kShadowCascades = 3;

/**
 * Cascaded directional shadow maps (3 cascades) as Texture2DArray.
 * Soft PCF is done in the lit shader.
 */
class DX12ShadowMap {
public:
    bool Create(ID3D12Device* device, u32 resolution,
                ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax);
    void Destroy();

    // cascadeIndex 0..2
    void BeginCascade(ID3D12GraphicsCommandList* cmd, u32 cascadeIndex);
    void EndAll(ID3D12GraphicsCommandList* cmd); // transition array to SRV

    void UpdateCascades(const Vec3& lightDir, const Vec3& focus,
                        f32 maxRadius = 40.0f);

    Mat4 GetLightViewProj(u32 cascade) const;
    const Mat4* GetAllLightVPs() const { return m_LightVP; }
    f32 GetCascadeSplit(u32 i) const { return m_Splits[i]; }

    D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSrvGpu() const { return m_SrvGpu; }
    u32 GetResolution() const { return m_Resolution; }
    bool IsValid() const { return m_Valid; }

private:
    ComPtr<ID3D12Resource> m_DepthArray;
    ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_DsvCpu[kShadowCascades] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_SrvCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_SrvGpu = {};
    Mat4 m_LightVP[kShadowCascades];
    f32 m_Splits[kShadowCascades] = { 8.f, 20.f, 40.f };
    u32 m_Resolution = 1024;
    bool m_Valid = false;
    bool m_InShaderReadable = false;
};

} // namespace Muk
