#include "DX12Pipeline.h"
#include "Core/Log.h"

#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace Muk {

static const char* g_EmbeddedHLSL = R"(
cbuffer FrameConstants : register(b0)
{
    float4x4 MVP;
};
struct VSInput {
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : COLOR;
};
struct PSInput {
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD;
};
PSInput VSMain(VSInput input) {
    PSInput o;
    o.Position = mul(float4(input.Position, 1.0f), MVP);
    o.Color = input.Color;
    o.TexCoord = input.TexCoord;
    return o;
}
float4 PSMain(PSInput input) : SV_TARGET {
    return input.Color;
}
)";

bool DX12Pipeline::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat) {
    if (!CreateRootSignature(device)) return false;
    if (!CreatePipelineState(device, rtvFormat, depthFormat)) return false;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = 256;
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_ConstantBuffer))))
        return false;

    m_ConstantBuffer->Map(0, nullptr, &m_CBMapped);
    Mat4 identity = Mat4::Identity();
    std::memcpy(m_CBMapped, identity.m, sizeof(float) * 16);

    m_Ready = true;
    MUK_CORE_INFO("DX12Pipeline ready (depth test + multi-mesh cache)");
    return true;
}

void DX12Pipeline::Shutdown() {
    if (m_ConstantBuffer && m_CBMapped) {
        m_ConstantBuffer->Unmap(0, nullptr);
        m_CBMapped = nullptr;
    }
    m_MeshCache.clear();
    m_ConstantBuffer.Reset();
    m_PipelineState.Reset();
    m_RootSignature.Reset();
    m_Ready = false;
}

bool DX12Pipeline::CreateRootSignature(ID3D12Device* device) {
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.Descriptor.ShaderRegister = 0;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = &rootParam;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature, error;
    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
        return false;

    return SUCCEEDED(device->CreateRootSignature(0, signature->GetBufferPointer(),
                      signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
}

bool DX12Pipeline::CompileShader(const char* entry, const char* target, ComPtr<ID3DBlob>& outBlob) {
    ComPtr<ID3DBlob> error;
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    HRESULT hr = D3DCompile(g_EmbeddedHLSL, std::strlen(g_EmbeddedHLSL), "Basic.hlsl",
                            nullptr, nullptr, entry, target, flags, 0, &outBlob, &error);
    if (FAILED(hr)) {
        if (error) MUK_CORE_ERROR("Shader error: {0}", (const char*)error->GetBufferPointer());
        return false;
    }
    return true;
}

bool DX12Pipeline::CreatePipelineState(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat) {
    ComPtr<ID3DBlob> vsBlob, psBlob;
    if (!CompileShader("VSMain", "vs_5_0", vsBlob)) return false;
    if (!CompileShader("PSMain", "ps_5_0", psBlob)) return false;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_RootSignature.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // Depth test ON
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DSVFormat = depthFormat;

    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = rtvFormat;
    psoDesc.SampleDesc.Count = 1;

    return SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)));
}

bool DX12Pipeline::HasMesh(const std::string& name) const {
    return m_MeshCache.find(name) != m_MeshCache.end();
}

bool DX12Pipeline::UploadMesh(ID3D12Device* device, const std::string& name, const Mesh& mesh) {
    if (HasMesh(name)) return true;

    const auto& vertices = mesh.GetVertices();
    const auto& indices = mesh.GetIndices();
    if (vertices.empty() || indices.empty()) return false;

    auto gpu = std::make_unique<GPUMeshBuffers>();
    const u64 vbSize = vertices.size() * sizeof(Vertex);
    const u64 ibSize = indices.size() * sizeof(u32);

    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    bufDesc.Width = vbSize;
    if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu->VertexBuffer))))
        return false;

    void* mapped = nullptr;
    gpu->VertexBuffer->Map(0, nullptr, &mapped);
    std::memcpy(mapped, vertices.data(), (size_t)vbSize);
    gpu->VertexBuffer->Unmap(0, nullptr);

    gpu->VBV.BufferLocation = gpu->VertexBuffer->GetGPUVirtualAddress();
    gpu->VBV.SizeInBytes = (UINT)vbSize;
    gpu->VBV.StrideInBytes = sizeof(Vertex);

    bufDesc.Width = ibSize;
    if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu->IndexBuffer))))
        return false;

    gpu->IndexBuffer->Map(0, nullptr, &mapped);
    std::memcpy(mapped, indices.data(), (size_t)ibSize);
    gpu->IndexBuffer->Unmap(0, nullptr);

    gpu->IBV.BufferLocation = gpu->IndexBuffer->GetGPUVirtualAddress();
    gpu->IBV.SizeInBytes = (UINT)ibSize;
    gpu->IBV.Format = DXGI_FORMAT_R32_UINT;
    gpu->IndexCount = (u32)indices.size();

    m_MeshCache[name] = std::move(gpu);
    MUK_CORE_INFO("GPU mesh cached: {0} ({1} verts)", name.c_str(), (int)vertices.size());
    return true;
}

void DX12Pipeline::Bind(ID3D12GraphicsCommandList* cmdList) {
    cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_PipelineState.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootConstantBufferView(0, m_ConstantBuffer->GetGPUVirtualAddress());
}

void DX12Pipeline::SetMVP(const Mat4& mvp) {
    if (m_CBMapped)
        std::memcpy(m_CBMapped, mvp.m, sizeof(float) * 16);
}

void DX12Pipeline::DrawMesh(ID3D12GraphicsCommandList* cmdList, const std::string& name) {
    auto it = m_MeshCache.find(name);
    if (it == m_MeshCache.end()) return;

    auto& gpu = *it->second;
    cmdList->IASetVertexBuffers(0, 1, &gpu.VBV);
    cmdList->IASetIndexBuffer(&gpu.IBV);
    cmdList->DrawIndexedInstanced(gpu.IndexCount, 1, 0, 0, 0);
}

} // namespace Muk
