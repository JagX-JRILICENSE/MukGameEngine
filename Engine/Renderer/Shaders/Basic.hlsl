cbuffer FrameConstants : register(b0)
{
    float4x4 MVP;
    float4   BaseColor;
    float    UseTexture;
    float3   Pad;
};

Texture2D    AlbedoTex : register(t0);
SamplerState AlbedoSam : register(s0);

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
    float2 TexCoord : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Position = mul(float4(input.Position, 1.0f), MVP);
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 tex = AlbedoTex.Sample(AlbedoSam, input.TexCoord);
    return lerp(input.Color * BaseColor, tex * BaseColor, UseTexture);
}
