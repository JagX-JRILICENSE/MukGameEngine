#include "DX12SkinPSO.h"
#include "Core/Log.h"
#include <d3dcompiler.h>
#include <vector>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

namespace Muk {

static const char* g_SkinHLSL = R"(
cbuffer FrameCB : register(b0) {
    float4x4 MVP;
    float4x4 World;
    float4 LightDir; // xyz + intensity
    float4 Ambient;  // rgb + pad
};
cbuffer SkinCB : register(b1) {
    float4x4 Bones[64];
    int BoneCount;
    int3 _pad;
};
struct VSIn {
    float3 Pos : POSITION;
    float3 Nrm : NORMAL;
    float2 UV : TEXCOORD;
    uint4  Idx : BLENDINDICES;
    float4 Wgt : BLENDWEIGHT;
};
struct VSOut {
    float4 Pos : SV_POSITION;
    float3 N : NORMAL;
};
float4x4 SkinM(uint4 i, float4 w) {
    float4x4 m = Bones[i.x] * w.x + Bones[i.y] * w.y + Bones[i.z] * w.z + Bones[i.w] * w.w;
    return m;
}
VSOut VSMain(VSIn v) {
    VSOut o;
    float4x4 s = SkinM(v.Idx, v.Wgt);
    float4 sk = mul(s, float4(v.Pos, 1));
    float4 wp = mul(World, sk);
    o.Pos = mul(MVP, wp);
    o.N = normalize(mul((float3x3)World, mul((float3x3)s, v.Nrm)));
    return o;
}
float4 PSMain(VSOut p) : SV_TARGET {
    float ndl = saturate(dot(normalize(p.N), normalize(-LightDir.xyz)));
    float3 c = float3(0.9, 0.7, 0.35) * (LightDir.w * ndl + Ambient.rgb);
    return float4(c, 1);
}
)";

bool DX12SkinPSO::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat, DXGI_FORMAT depthFormat) {
    if (!CreateRS(device) || !CreatePSO(device, rtvFormat, depthFormat)) return false;

    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Height = 1; rd.DepthOrArraySize = 1; rd.MipLevels = 1;
    rd.SampleDesc.Count = 1; rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    rd.Width = 512;
    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_FrameCB)))) return false;
    m_FrameCB->Map(0, nullptr, &m_FrameMapped);

    rd.Width = sizeof(SkinCBData) + 256;
    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_BoneCB)))) return false;
    m_BoneCB->Map(0, nullptr, &m_BoneMapped);

    UploadSkinnedCube(device);
    m_Ready = true;
    MUK_CORE_INFO("DX12SkinPSO ready (GPU skinning)");
    return true;
}

void DX12SkinPSO::Shutdown() {
    if (m_FrameCB && m_FrameMapped) { m_FrameCB->Unmap(0, nullptr); m_FrameMapped = nullptr; }
    if (m_BoneCB && m_BoneMapped) { m_BoneCB->Unmap(0, nullptr); m_BoneMapped = nullptr; }
    m_Meshes.clear();
    m_FrameCB.Reset(); m_BoneCB.Reset(); m_PSO.Reset(); m_RS.Reset();
    m_Ready = false;
}

bool DX12SkinPSO::CreateRS(ID3D12Device* device) {
    D3D12_ROOT_PARAMETER p[2] = {};
    p[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    p[0].Descriptor.ShaderRegister = 0;
    p[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    p[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    p[1].Descriptor.ShaderRegister = 1;
    p[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rs = {};
    rs.NumParameters = 2; rs.pParameters = p;
    rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> sig, err;
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err))) return false;
    return SUCCEEDED(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_RS)));
}

bool DX12SkinPSO::CreatePSO(ID3D12Device* device, DXGI_FORMAT rtv, DXGI_FORMAT dsv) {
    ComPtr<ID3DBlob> vs, ps, err;
    if (FAILED(D3DCompile(g_SkinHLSL, strlen(g_SkinHLSL), "skin", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vs, &err))) {
        if (err) MUK_CORE_ERROR("{0}", (const char*)err->GetBufferPointer());
        return false;
    }
    if (FAILED(D3DCompile(g_SkinHLSL, strlen(g_SkinHLSL), "skin", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &ps, &err)))
        return false;

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = m_RS.Get();
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
    pso.DSVFormat = dsv;
    pso.InputLayout = { layout, _countof(layout) };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = rtv;
    pso.SampleDesc.Count = 1;
    return SUCCEEDED(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_PSO)));
}

bool DX12SkinPSO::UploadSkinnedCube(ID3D12Device* device) {
    // 8 corners, half weighted to bone 0, half to bone 1
    struct V { float p[3], n[3], uv[2]; uint32_t i[4]; float w[4]; };
    std::vector<V> verts;
    auto add = [&](float x, float y, float z, int bone) {
        V v{};
        v.p[0]=x; v.p[1]=y; v.p[2]=z; v.n[1]=1; v.uv[0]=0; v.uv[1]=0;
        v.i[0]=bone; v.w[0]=1.f;
        verts.push_back(v);
    };
    float s = 0.3f;
    // lower bone 0
    add(-s,0,-s,0); add(s,0,-s,0); add(s,0.6f,-s,0); add(-s,0.6f,-s,0);
    add(-s,0,s,0); add(s,0,s,0); add(s,0.6f,s,0); add(-s,0.6f,s,0);
    // upper bone 1
    add(-s,0.6f,-s,1); add(s,0.6f,-s,1); add(s,1.2f,-s,1); add(-s,1.2f,-s,1);
    add(-s,0.6f,s,1); add(s,0.6f,s,1); add(s,1.2f,s,1); add(-s,1.2f,s,1);

    std::vector<uint32_t> idx;
    auto box = [&](uint32_t b) {
        const uint32_t q[] = {0,1,2,0,2,3, 4,6,5,4,7,6, 0,4,5,0,5,1, 2,6,7,2,7,3, 0,3,7,0,7,4, 1,5,6,1,6,2};
        for (auto i : q) idx.push_back(b + i);
    };
    box(0); box(8);

    auto gpu = std::make_unique<GPUSkinnedMesh>();
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Height = 1; rd.DepthOrArraySize = 1; rd.MipLevels = 1;
    rd.SampleDesc.Count = 1; rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Width = verts.size() * sizeof(V);
    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu->VB)))) return false;
    void* m = nullptr; gpu->VB->Map(0, nullptr, &m); memcpy(m, verts.data(), (size_t)rd.Width); gpu->VB->Unmap(0, nullptr);
    gpu->VBV.BufferLocation = gpu->VB->GetGPUVirtualAddress();
    gpu->VBV.SizeInBytes = (UINT)rd.Width;
    gpu->VBV.StrideInBytes = sizeof(V);

    rd.Width = idx.size() * sizeof(uint32_t);
    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu->IB)))) return false;
    gpu->IB->Map(0, nullptr, &m); memcpy(m, idx.data(), (size_t)rd.Width); gpu->IB->Unmap(0, nullptr);
    gpu->IBV.BufferLocation = gpu->IB->GetGPUVirtualAddress();
    gpu->IBV.SizeInBytes = (UINT)rd.Width;
    gpu->IBV.Format = DXGI_FORMAT_R32_UINT;
    gpu->IndexCount = (u32)idx.size();
    m_Meshes["SkinnedCube"] = std::move(gpu);
    return true;
}

void DX12SkinPSO::Bind(ID3D12GraphicsCommandList* cmd) {
    cmd->SetGraphicsRootSignature(m_RS.Get());
    cmd->SetPipelineState(m_PSO.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->SetGraphicsRootConstantBufferView(0, m_FrameCB->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(1, m_BoneCB->GetGPUVirtualAddress());
}

void DX12SkinPSO::SetBones(const SkinCBData& skin) {
    if (m_BoneMapped) std::memcpy(m_BoneMapped, &skin, sizeof(SkinCBData));
}

void DX12SkinPSO::SetWorldViewProj(const Mat4& mvp, const Mat4& world,
                                   const float lightDir[3], float intensity, float ambient) {
    if (!m_FrameMapped) return;
    struct { float MVP[16]; float World[16]; float LD[4]; float Amb[4]; } cb{};
    std::memcpy(cb.MVP, mvp.m, 64);
    std::memcpy(cb.World, world.m, 64);
    cb.LD[0]=lightDir[0]; cb.LD[1]=lightDir[1]; cb.LD[2]=lightDir[2]; cb.LD[3]=intensity;
    cb.Amb[0]=cb.Amb[1]=cb.Amb[2]=ambient;
    std::memcpy(m_FrameMapped, &cb, sizeof(cb));
}

void DX12SkinPSO::Draw(ID3D12GraphicsCommandList* cmd, const std::string& name) {
    auto it = m_Meshes.find(name);
    if (it == m_Meshes.end()) return;
    auto& g = *it->second;
    cmd->IASetVertexBuffers(0, 1, &g.VBV);
    cmd->IASetIndexBuffer(&g.IBV);
    cmd->DrawIndexedInstanced(g.IndexCount, 1, 0, 0, 0);
}

} // namespace Muk
