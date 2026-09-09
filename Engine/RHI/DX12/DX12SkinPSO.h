#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Core/Core.h"
#include "Math/Matrix.h"
#include "Animation/SkinGPU.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <cstring>

namespace Muk {

using Microsoft::WRL::ComPtr;

struct GPUSkinnedMesh {
    ComPtr<ID3D12Resource> VB;
    ComPtr<ID3D12Resource> IB;
    D3D12_VERTEX_BUFFER_VIEW VBV{};
    D3D12_INDEX_BUFFER_VIEW IBV{};
    u32 IndexCount = 0;
};

/** Dedicated root signature + PSO for GPU skinning (b0 frame, b1 bones) */
class DX12SkinPSO {
public:
    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat);
    void Shutdown();

    void Bind(ID3D12GraphicsCommandList* cmd);
    void SetBones(const SkinCBData& skin);
    void SetWorldViewProj(const Mat4& mvp, const Mat4& world,
                          const float lightDir[3], float intensity, float ambient);

    bool UploadSkinnedCube(ID3D12Device* device); // demo mesh with 2-bone weights
    void Draw(ID3D12GraphicsCommandList* cmd, const std::string& name = "SkinnedCube");

    bool IsReady() const { return m_Ready; }

private:
    bool CreateRS(ID3D12Device* device);
    bool CreatePSO(ID3D12Device* device, DXGI_FORMAT rtv, DXGI_FORMAT dsv);

    ComPtr<ID3D12RootSignature> m_RS;
    ComPtr<ID3D12PipelineState> m_PSO;
    ComPtr<ID3D12Resource> m_FrameCB;
    ComPtr<ID3D12Resource> m_BoneCB;
    void* m_FrameMapped = nullptr;
    void* m_BoneMapped = nullptr;
    std::unordered_map<std::string, std::unique_ptr<GPUSkinnedMesh>> m_Meshes;
    bool m_Ready = false;
};

} // namespace Muk
