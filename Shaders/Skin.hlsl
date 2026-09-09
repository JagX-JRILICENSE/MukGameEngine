// Muk Game Engine — skinned mesh VS/PS
// SkinCB b1: 64 bone matrices

cbuffer FrameCB : register(b0) {
    float4x4 ViewProj;
    float4x4 World;
    float3 LightDir;
    float Ambient;
    float3 LightColor;
    float Intensity;
};

cbuffer SkinCB : register(b1) {
    float4x4 Bones[64];
    int BoneCount;
    int3 _pad;
};

struct VSIn {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    uint4 BlendIndices : BLENDINDICES;
    float4 BlendWeights : BLENDWEIGHT;
};

struct VSOut {
    float4 Pos : SV_POSITION;
    float3 N : NORMAL;
    float2 UV : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

float4x4 SkinMatrix(uint4 idx, float4 w) {
    float4x4 m = Bones[idx.x] * w.x;
    m += Bones[idx.y] * w.y;
    m += Bones[idx.z] * w.z;
    m += Bones[idx.w] * w.w;
    return m;
}

VSOut VSMain(VSIn vin) {
    VSOut o;
    float4x4 skin = SkinMatrix(vin.BlendIndices, vin.BlendWeights);
    float4 skinned = mul(skin, float4(vin.Position, 1));
    float4 worldPos = mul(World, skinned);
    o.Pos = mul(ViewProj, worldPos);
    o.WorldPos = worldPos.xyz;
    float3 n = mul((float3x3)skin, vin.Normal);
    o.N = normalize(mul((float3x3)World, n));
    o.UV = vin.UV;
    return o;
}

float4 PSMain(VSOut pin) : SV_TARGET {
    float3 n = normalize(pin.N);
    float ndl = saturate(dot(n, normalize(-LightDir)));
    float3 col = LightColor * Intensity * ndl + Ambient;
    return float4(col, 1);
}
