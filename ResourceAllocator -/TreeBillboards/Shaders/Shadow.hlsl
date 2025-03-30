#ifndef __SHADOW__H__
#define __SHADOW__H__

#include "DSMCG.hlsli"
#include "DSMLighting.hlsli"

ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);
ConstantBuffer<MaterialConstants> gMatCB : register(b2);

Texture2D gDiffuse : register(t0);

SamplerState gSamplerAnisotropicWrap : register(s2);

VertexPosHTex ShadowVS(VertexPosLNormalLTex i)
{
    VertexPosHTex o;
    float4x4 viewProj = mul(gPassCB.View, gPassCB.Proj);
    float4 posW = mul(float4(i.PosL, 1), gObjCB.World);
    o.PosH = mul(posW, viewProj);
    o.TexCoord = i.TexCoord;
    return o;
}

// 当使用深度测试的时候，需要将透明的部分裁剪掉来保证阴影的正确
void ShadowPS(VertexPosHTex i)
{
    float4 texCol = gDiffuse.Sample(gSamplerAnisotropicWrap, i.TexCoord);
    float alpha = texCol.a * gMatCB.Alpha;

#ifdef ALPHATEST
    clip(alpha - 0.1f);
#endif
}

float4 ShadowDebugPS(VertexPosHTex i)
{
    float depth = gDiffuse.Sample(gSamplerAnisotropicWrap, i.TexCoord).r;
    return float4(depth.rrr, 1.0f);
}


#endif