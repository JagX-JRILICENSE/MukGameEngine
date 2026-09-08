#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "Core/Core.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "DX12Texture.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

struct GPUMeshBuffers {
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VBV = {};
    D3D12_INDEX_BUFFER_VIEW IBV = {};
    u32 IndexCount = 0;
};

struct FrameCB {
    float MVP[16];
    float World[16];
    float LightVP[16];
    float BaseColor[4];
    float LightDir[4];
    float LightColor[4];
    float UseTexture;
    float Metallic;
    float Roughness;
    float ReceiveShadows; // 1 = sample shadow map
};

class DX12Pipeline {
public:
    bool Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat,
                    ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax);
    void Shutdown();

    bool UploadMesh(ID3D12Device* device, const std::string& name, const Mesh& mesh);
    bool UploadTexture(ID3D12Device* device, const std::string& name, const Texture& tex);
    bool HasMesh(const std::string& name) const;

    void BindLit(ID3D12GraphicsCommandList* cmdList);
    void BindShadow(ID3D12GraphicsCommandList* cmdList);

    void SetDrawParams(const Mat4& mvp, const Mat4& world, const Mat4& lightVP,
                       const Material& material,
                       const Vec3& lightDir, const Vec3& lightColor, f32 intensity, f32 ambient,
                       bool receiveShadows);

    void DrawMesh(ID3D12GraphicsCommandList* cmdList, const std::string& name);

    bool IsReady() const { return m_Ready; }
    DX12TextureCache& Textures() { return m_Textures; }

private:
    bool CreateRootSignature(ID3D12Device* device);
    bool CreateLitPSO(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat);
    bool CreateShadowPSO(ID3D12Device* device);
    bool CompileShader(const char* source, const char* entry, const char* target, ComPtr<ID3DBlob>& outBlob);

    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_LitPSO;
    ComPtr<ID3D12PipelineState> m_ShadowPSO;
    ComPtr<ID3D12Resource> m_ConstantBuffer;
    void* m_CBMapped = nullptr;

    std::unordered_map<std::string, std::unique_ptr<GPUMeshBuffers>> m_MeshCache;
    DX12TextureCache m_Textures;
    bool m_Ready = false;
};

} // namespace Muk
