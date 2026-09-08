#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "Core/Core.h"
#include "Renderer/Mesh.h"
#include "Math/Matrix.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

struct GPUMeshBuffers {
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VBV = {};
    D3D12_INDEX_BUFFER_VIEW IBV = {};
    u32 IndexCount = 0;
};

/**
 * Root signature, depth-tested PSO, constant buffer, multi-mesh GPU cache.
 */
class DX12Pipeline {
public:
    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat);
    void Shutdown();

    // Cache by name - returns true if newly uploaded or already cached
    bool UploadMesh(ID3D12Device* device, const std::string& name, const Mesh& mesh);
    bool HasMesh(const std::string& name) const;

    void Bind(ID3D12GraphicsCommandList* cmdList);
    void SetMVP(const Mat4& mvp);
    void DrawMesh(ID3D12GraphicsCommandList* cmdList, const std::string& name);

    bool IsReady() const { return m_Ready; }

private:
    bool CreateRootSignature(ID3D12Device* device);
    bool CreatePipelineState(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat);
    bool CompileShader(const char* entry, const char* target, ComPtr<ID3DBlob>& outBlob);

    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
    ComPtr<ID3D12Resource> m_ConstantBuffer;
    void* m_CBMapped = nullptr;

    std::unordered_map<std::string, std::unique_ptr<GPUMeshBuffers>> m_MeshCache;
    bool m_Ready = false;
};

} // namespace Muk
