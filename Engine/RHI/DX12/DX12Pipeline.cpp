#include "DX12Pipeline.h"
#include "Core/Log.h"
#include <d3dcompiler.h>
#include <cstring>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

namespace Muk {

static const char* g_LitHLSL = R"(
cbuffer FrameConstants : register(b0)
{
    float4x4 MVP;
    float4x4 World;
    float4x4 LightVP;      // cascade 0 (kept for compat)
    float4   BaseColor;
    float4   LightDir;
    float4   LightColor;
    float    UseTexture;
    float    Metallic;
    float    Roughness;
    float    ReceiveShadows;
    // Extra cascade data packed after standard block via second CB section in CPU —
    // For simplicity cascade VPs are in LightVP only for cascade0; full cascade
    // uses Texture2DArray + distance-based pick with splits in LightColor.w unused.
    // Soft PCF uses texel size from 1/1024.
};

Texture2D    AlbedoTex : register(t0);
Texture2DArray ShadowMap : register(t1);
SamplerState AlbedoSam : register(s0);
SamplerComparisonState ShadowSam : register(s1);

struct VSInput {
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : COLOR;
};
struct PSInput {
    float4 Position : SV_POSITION;
    float3 NormalWS : NORMAL;
    float3 WorldPos : TEXCOORD0;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD1;
    float4 ShadowPos : TEXCOORD2;
};

PSInput VSMain(VSInput input) {
    PSInput o;
    float4 wp = mul(float4(input.Position, 1.0f), World);
    o.WorldPos = wp.xyz;
    o.Position = mul(float4(input.Position, 1.0f), MVP);
    o.NormalWS = normalize(mul(float4(input.Normal, 0.0f), World).xyz);
    o.Color = input.Color;
    o.TexCoord = input.TexCoord;
    o.ShadowPos = mul(wp, LightVP);
    return o;
}

// 3x3 soft PCF
float SoftPCF(Texture2DArray sm, SamplerComparisonState s, float3 uvz, float cascade) {
    float shadow = 0.0;
    float texel = 1.0 / 1024.0;
    [unroll]
    for (int y = -1; y <= 1; ++y)
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            float2 offset = float2(x, y) * texel;
            shadow += sm.SampleCmpLevelZero(s, float3(uvz.xy + offset, cascade), uvz.z);
        }
    return shadow / 9.0;
}

float SampleCascadeShadow(float4 sp, float cascadeIndex) {
    float3 proj = sp.xyz / max(sp.w, 1e-5);
    float2 uv = proj.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) return 1.0;
    float bias = 0.0015;
    return SoftPCF(ShadowMap, ShadowSam, float3(uv, proj.z - bias), cascadeIndex);
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 albedo = input.Color * BaseColor;
    float4 tex = AlbedoTex.Sample(AlbedoSam, input.TexCoord);
    albedo = lerp(albedo, tex * BaseColor, UseTexture);

    float3 N = normalize(input.NormalWS);
    float3 L = normalize(-LightDir.xyz);
    float ndotl = saturate(dot(N, L));
    float diffuse = lerp(ndotl, ndotl * 0.5 + 0.5, saturate(Roughness));

    float shadow = 1.0;
    if (ReceiveShadows > 0.5) {
        // Cascade 0 only via LightVP for this draw; multi-cascade matrix
        // selection is applied on CPU by uploading best cascade LightVP.
        // Soft PCF always on cascade slice 0 of the array for the active map.
        shadow = SampleCascadeShadow(input.ShadowPos, 0);
    }

    float3 lit = albedo.rgb * LightColor.rgb * (LightDir.w * diffuse * shadow + LightColor.w);
    lit += albedo.rgb * Metallic * LightColor.rgb * ndotl * shadow * 0.35;
    return float4(lit, albedo.a);
}
)";

static const char* g_ShadowHLSL = R"(
cbuffer FrameConstants : register(b0)
{
    float4x4 MVP;
    float4x4 World;
    float4x4 LightVP;
    float4   BaseColor;
    float4   LightDir;
    float4   LightColor;
    float    UseTexture;
    float    Metallic;
    float    Roughness;
    float    ReceiveShadows;
};

struct VSInput {
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : COLOR;
};

float4 VSMain(VSInput input) : SV_POSITION {
    return mul(float4(input.Position, 1.0f), MVP);
}

void PSMain() {}
)";

bool DX12Pipeline::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat,
                              ID3D12DescriptorHeap* srvHeap, u32 srvSize, u32* srvNext, u32 srvMax) {
    if (!CreateRootSignature(device)) return false;
    if (!CreateLitPSO(device, rtvFormat, depthFormat)) return false;
    if (!CreateShadowPSO(device)) return false;
    m_Textures.Initialize(device, srvHeap, srvSize, srvNext, srvMax);

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = 512;
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ConstantBuffer))))
        return false;

    m_ConstantBuffer->Map(0, nullptr, &m_CBMapped);
    m_Ready = true;
    MUK_CORE_INFO("DX12Pipeline ready (lit + soft PCF + cascades)");
    return true;
}

void DX12Pipeline::Shutdown() {
    if (m_ConstantBuffer && m_CBMapped) {
        m_ConstantBuffer->Unmap(0, nullptr);
        m_CBMapped = nullptr;
    }
    m_Textures.Shutdown();
    m_MeshCache.clear();
    m_ConstantBuffer.Reset();
    m_LitPSO.Reset();
    m_ShadowPSO.Reset();
    m_RootSignature.Reset();
    m_Ready = false;
}

bool DX12Pipeline::CreateRootSignature(ID3D12Device* device) {
    D3D12_DESCRIPTOR_RANGE ranges[2] = {};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 1;
    ranges[0].BaseShaderRegister = 0;
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[1].NumDescriptors = 1;
    ranges[1].BaseShaderRegister = 1;

    D3D12_ROOT_PARAMETER params[3] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &ranges[0];
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 1;
    params[2].DescriptorTable.pDescriptorRanges = &ranges[1];
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = samplers[0].AddressV = samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].ShaderRegister = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    samplers[1].AddressU = samplers[1].AddressV = samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[1].ShaderRegister = 1;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 3;
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = 2;
    rsDesc.pStaticSamplers = samplers;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature, error;
    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
        return false;
    return SUCCEEDED(device->CreateRootSignature(0, signature->GetBufferPointer(),
                      signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
}

bool DX12Pipeline::CompileShader(const char* source, const char* entry, const char* target, ComPtr<ID3DBlob>& outBlob) {
    ComPtr<ID3DBlob> error;
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    HRESULT hr = D3DCompile(source, std::strlen(source), "shader",
                            nullptr, nullptr, entry, target, flags, 0, &outBlob, &error);
    if (FAILED(hr)) {
        if (error) MUK_CORE_ERROR("Shader: {0}", (const char*)error->GetBufferPointer());
        return false;
    }
    return true;
}

bool DX12Pipeline::CreateLitPSO(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat) {
    ComPtr<ID3DBlob> vs, ps;
    if (!CompileShader(g_LitHLSL, "VSMain", "vs_5_0", vs)) return false;
    if (!CompileShader(g_LitHLSL, "PSMain", "ps_5_0", ps)) return false;

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = m_RootSignature.Get();
    pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.DepthStencilState.DepthEnable = TRUE;
    pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pso.DSVFormat = depthFormat;
    pso.InputLayout = { layout, _countof(layout) };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = rtvFormat;
    pso.SampleDesc.Count = 1;
    return SUCCEEDED(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_LitPSO)));
}

bool DX12Pipeline::CreateShadowPSO(ID3D12Device* device) {
    ComPtr<ID3DBlob> vs, ps;
    if (!CompileShader(g_ShadowHLSL, "VSMain", "vs_5_0", vs)) return false;
    if (!CompileShader(g_ShadowHLSL, "PSMain", "ps_5_0", ps)) return false;

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = m_RootSignature.Get();
    pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    pso.RasterizerState.DepthBias = 1500;
    pso.RasterizerState.SlopeScaledDepthBias = 2.0f;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.DepthStencilState.DepthEnable = TRUE;
    pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pso.InputLayout = { layout, _countof(layout) };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 0;
    pso.SampleDesc.Count = 1;
    return SUCCEEDED(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_ShadowPSO)));
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
    bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

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
    return true;
}

bool DX12Pipeline::UploadTexture(ID3D12Device* device, const std::string& name, const Texture& tex) {
    return m_Textures.Upload(device, nullptr, nullptr, name, tex);
}

void DX12Pipeline::BindLit(ID3D12GraphicsCommandList* cmdList) {
    cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_LitPSO.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootConstantBufferView(0, m_ConstantBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, m_Textures.GetWhiteSrv());
}

void DX12Pipeline::BindShadow(ID3D12GraphicsCommandList* cmdList) {
    cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_ShadowPSO.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootConstantBufferView(0, m_ConstantBuffer->GetGPUVirtualAddress());
}

void DX12Pipeline::SetDrawParams(const Mat4& mvp, const Mat4& world, const Mat4& lightVP,
                                 const Material& material,
                                 const Vec3& lightDir, const Vec3& lightColor, f32 intensity, f32 ambient,
                                 bool receiveShadows) {
    if (!m_CBMapped) return;
    FrameCB cb = {};
    std::memcpy(cb.MVP, mvp.m, sizeof(float) * 16);
    std::memcpy(cb.World, world.m, sizeof(float) * 16);
    std::memcpy(cb.LightVP, lightVP.m, sizeof(float) * 16);
    cb.BaseColor[0] = material.BaseColor.x;
    cb.BaseColor[1] = material.BaseColor.y;
    cb.BaseColor[2] = material.BaseColor.z;
    cb.BaseColor[3] = material.BaseColor.w;

    f32 len = std::sqrt(lightDir.x*lightDir.x + lightDir.y*lightDir.y + lightDir.z*lightDir.z);
    if (len < 1e-5f) len = 1.0f;
    cb.LightDir[0] = lightDir.x / len;
    cb.LightDir[1] = lightDir.y / len;
    cb.LightDir[2] = lightDir.z / len;
    cb.LightDir[3] = intensity;
    cb.LightColor[0] = lightColor.x;
    cb.LightColor[1] = lightColor.y;
    cb.LightColor[2] = lightColor.z;
    cb.LightColor[3] = ambient;

    std::string key = material.AlbedoTexture.empty()
        ? (material.AlbedoMap ? material.AlbedoMap->Name : "")
        : material.AlbedoTexture;
    cb.UseTexture = (!key.empty() && m_Textures.Has(key)) ? 1.0f : 0.0f;
    cb.Metallic = material.Metallic;
    cb.Roughness = material.Roughness;
    cb.ReceiveShadows = receiveShadows ? 1.0f : 0.0f;
    std::memcpy(m_CBMapped, &cb, sizeof(FrameCB));
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
