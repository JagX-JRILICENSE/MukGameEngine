// Muk Game Engine - Basic color shader
// Vertex color passthrough with simple MVP transform

cbuffer FrameConstants : register(b0)
{
    float4x4 MVP;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : COLOR;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.Color = input.Color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.Color;
}
