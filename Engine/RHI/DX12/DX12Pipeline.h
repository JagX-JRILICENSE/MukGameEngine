#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include "Core/Core.h"
#include "Renderer/Mesh.h"
#include "Math/Matrix.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

/**
 * Owns root signature, PSO, and per-mesh GPU buffers for the basic triangle path.
 */
class DX12Pipeline {
public:
    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat);
    void Shutdown();

    // Upload mesh data to GPU (simple single-buffer for now)
    bool UploadMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
                    ID3D12CommandQueue* queue, const Mesh& mesh);

    void Bind(ID3D12GraphicsCommandList* cmdList);
    void SetMVP(ID3D12GraphicsCommandList* cmdList, const Mat4& mvp);
    void Draw(ID3D12GraphicsCommandList* cmdList);

    bool IsReady() const { return m_Ready; }

private:
    bool CreateRootSignature(ID3D12Device* device);
    bool CreatePipelineState(ID3D12Device* device, DXGI_FORMAT rtvFormat);
    bool CompileShader(const std::wstring& path, const char* entry, const char* target,
                       ComPtr<ID3DBlob>& outBlob);

    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
    ComPtr<ID3D12Resource> m_VertexBuffer;
    ComPtr<ID3D12Resource> m_IndexBuffer;
    ComPtr<ID3D12Resource> m_ConstantBuffer;

    D3D12_VERTEX_BUFFER_VIEW m_VBV = {};
    D3D12_INDEX_BUFFER_VIEW m_IBV = {};
    u32 m_IndexCount = 0;
    bool m_Ready = false;

    // Mapped constant buffer (MVP matrix)
    void* m_CBMapped = nullptr;
};

} // namespace Muk
