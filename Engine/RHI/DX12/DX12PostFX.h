#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Core/Core.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

/** Fullscreen post: SSAO (depth-aware approx) + bloom bright-pass + blur composite */
class DX12PostFX {
public:
    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat);
    void Shutdown();

    void SetParams(float ssaoIntensity, float ssaoRadius, float bloomStrength, float bloomThreshold,
                   float exposure, bool enableSSAO, bool enableBloom);

    /** Bind fullscreen triangle pipeline and draw (expects scene color as t0 SRV) */
    void Draw(ID3D12GraphicsCommandList* cmd, D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv);

    bool IsReady() const { return m_Ready; }

private:
    bool CreatePSO(ID3D12Device* device, DXGI_FORMAT rtvFormat);
    bool Compile(const char* src, const char* entry, const char* target, ComPtr<ID3DBlob>& out);

    ComPtr<ID3D12RootSignature> m_RS;
    ComPtr<ID3D12PipelineState> m_PSO;
    ComPtr<ID3D12Resource> m_CB;
    void* m_CBMapped = nullptr;
    bool m_Ready = false;
};

} // namespace Muk
