#include "DSMCG.hlsli"
#include "DSMLighting.hlsli"

ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);
ConstantBuffer<Lights> gLightCB : register(b2);
ConstantBuffer<MaterialConstants> gMatCB : register(b3);

Texture2D gDiffuse : register(t0);
SamplerState gSamplerPoint : register(s0);

VertexPosHColorTex VS(VertexPosLNormalLTex v)
{
    VertexPosHColorTex o;
    float4x4 viewProj = mul(gPassCB.View, gPassCB.Proj);
    float4 posW = mul(float4(v.PosL, 1), gObjCB.World);
    o.PosH = mul(posW, viewProj);
    o.Color = float4(1,1,1,1);
    o.TexCoord = v.TexCoord;
    return o;
}

float4 PS(VertexPosHColorTex i) : SV_Target
{
    return gDiffuse.Sample(gSamplerPoint, i.TexCoord) * i.Color;
}