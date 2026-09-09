#include "DX12PostFX.h"
#include "Core/Log.h"
#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace Muk {

struct PostCB {
    float SSAOIntensity;
    float SSAORadius;
    float BloomStrength;
    float BloomThreshold;
    float Exposure;
    float EnableSSAO;
    float EnableBloom;
    float _pad;
};

static const char* g_PostHLSL = R"(
cbuffer PostCB : register(b0) {
    float SSAOIntensity, SSAORadius, BloomStrength, BloomThreshold;
    float Exposure, EnableSSAO, EnableBloom, _pad;
};
Texture2D SceneTex : register(t0);
SamplerState LinearSam : register(s0);

struct VSOut { float4 Pos : SV_POSITION; float2 UV : TEXCOORD0; };

VSOut VSMain(uint id : SV_VertexID) {
    VSOut o;
    o.UV = float2((id << 1) & 2, id & 2);
    o.Pos = float4(o.UV * float2(2, -2) + float2(-1, 1), 0, 1);
    return o;
}

float Luma(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

float4 PSMain(VSOut i) : SV_TARGET {
    float2 uv = i.UV;
    float3 col = SceneTex.Sample(LinearSam, uv).rgb;

    // Cheap SSAO: sample neighborhood luminance as occlusion proxy
    if (EnableSSAO > 0.5) {
        float occ = 0;
        float base = Luma(col);
        [unroll] for (int y = -1; y <= 1; ++y)
            [unroll] for (int x = -1; x <= 1; ++x) {
                float2 o = float2(x, y) * SSAORadius * 0.002;
                float n = Luma(SceneTex.Sample(LinearSam, uv + o).rgb);
                occ += saturate(base - n + 0.02);
            }
        occ = 1.0 - saturate(occ / 9.0) * SSAOIntensity;
        col *= occ;
    }

    // Bloom: extract + soft blur
    float3 bloom = 0;
    if (EnableBloom > 0.5) {
        [unroll] for (int by = -2; by <= 2; ++by)
            [unroll] for (int bx = -2; bx <= 2; ++bx) {
                float2 o = float2(bx, by) * 0.003;
                float3 s = SceneTex.Sample(LinearSam, uv + o).rgb;
                float l = Luma(s);
                bloom += s * saturate(l - BloomThreshold);
            }
        bloom = (bloom / 25.0) * BloomStrength;
        col += bloom;
    }

    col *= max(Exposure, 0.01);
    col = col / (col + 1.0); // Reinhard
    return float4(col, 1);
}
)";

bool DX12PostFX::Compile(const char* src, const char* entry, const char* target, ComPtr<ID3DBlob>& out) {
    ComPtr<ID3DBlob> err;
    HRESULT hr = D3DCompile(src, strlen(src), "post", nullptr, nullptr, entry, target, 0, 0, &out, &err);
    if (FAILED(hr)) {
        if (err) MUK_CORE_ERROR("PostFX: {0}", (const char*)err->GetBufferPointer());
        return false;
    }
    return true;
}

bool DX12PostFX::CreatePSO(ID3D12Device* device, DXGI_FORMAT rtvFormat) {
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER params[2] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &range;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sam = {};
    sam.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sam.AddressU = sam.AddressV = sam.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sam.ShaderRegister = 0;
    sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rs = {};
    rs.NumParameters = 2; rs.pParameters = params;
    rs.NumStaticSamplers = 1; rs.pStaticSamplers = &sam;
    rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sig, error;
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error)))
        return false;
    if (FAILED(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_RS))))
        return false;

    ComPtr<ID3DBlob> vs, ps;
    if (!Compile(g_PostHLSL, "VSMain", "vs_5_0", vs)) return false;
    if (!Compile(g_PostHLSL, "PSMain", "ps_5_0", ps)) return false;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = m_RS.Get();
    pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso.DepthStencilState.DepthEnable = FALSE;
    pso.InputLayout = { nullptr, 0 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = rtvFormat;
    pso.SampleDesc.Count = 1;
    return SUCCEEDED(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_PSO)));
}

bool DX12PostFX::Initialize(ID3D12Device* device, DXGI_FORMAT rtvFormat) {
    if (!CreatePSO(device, rtvFormat)) return false;
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = 256; rd.Height = 1; rd.DepthOrArraySize = 1; rd.MipLevels = 1;
    rd.SampleDesc.Count = 1; rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_CB))))
        return false;
    m_CB->Map(0, nullptr, &m_CBMapped);
    SetParams(0.85f, 1.0f, 0.45f, 1.0f, 1.15f, true, true);
    m_Ready = true;
    MUK_CORE_INFO("DX12PostFX ready (SSAO + bloom fullscreen)");
    return true;
}

void DX12PostFX::Shutdown() {
    if (m_CB && m_CBMapped) { m_CB->Unmap(0, nullptr); m_CBMapped = nullptr; }
    m_CB.Reset(); m_PSO.Reset(); m_RS.Reset(); m_Ready = false;
}

void DX12PostFX::SetParams(float ssaoIntensity, float ssaoRadius, float bloomStrength, float bloomThreshold,
                           float exposure, bool enableSSAO, bool enableBloom) {
    if (!m_CBMapped) return;
    PostCB cb{};
    cb.SSAOIntensity = ssaoIntensity; cb.SSAORadius = ssaoRadius;
    cb.BloomStrength = bloomStrength; cb.BloomThreshold = bloomThreshold;
    cb.Exposure = exposure;
    cb.EnableSSAO = enableSSAO ? 1.f : 0.f;
    cb.EnableBloom = enableBloom ? 1.f : 0.f;
    std::memcpy(m_CBMapped, &cb, sizeof(cb));
}

void DX12PostFX::Draw(ID3D12GraphicsCommandList* cmd, D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv) {
    if (!m_Ready || !cmd) return;
    cmd->SetGraphicsRootSignature(m_RS.Get());
    cmd->SetPipelineState(m_PSO.Get());
    cmd->SetGraphicsRootConstantBufferView(0, m_CB->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(1, sceneSrv);
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->DrawInstanced(3, 1, 0, 0);
}

} // namespace Muk
