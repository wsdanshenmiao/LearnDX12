#ifndef __CYLINDER__HLSL__
#define __CYLINDER__HLSL__

#include "DSMCG.hlsli"
#include "DSMLighting.hlsli"

ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);
ConstantBuffer<Lights> gLightCB : register(b2);
ConstantBuffer<MaterialConstants> gMatCB : register(b3);
cbuffer gCylinderCB : register(b2)
{
    float CylinderHeight;
    float3 Pad;
}

Texture2D gDiffuse : register(t0);

SamplerState gSamplerAnisotropicWrap    : register(s2);

VertexPosWHNormalWColor CylinderVS(VertexPosLNormalLColor v)
{
    VertexPosWHNormalWColor o;
    float4x4 viewProj = mul(gPassCB.View, gPassCB.Proj);
    float4 posW = mul(float4(v.PosL, 1), gObjCB.World);
    o.PosH = mul(posW, viewProj);
    o.PosW = posW.xyz;
    o.NormalW = mul(float4(v.NormalL, 1), gObjCB.WorldInvTranspose).xyz;
    o.Color = v.Color;
    return o;
}

float4 CylinderPS(VertexPosWHNormalWColor i) : SV_Target
{
    float3 viewDir = i.PosW - gPassCB.EyePosW;
    float distToEye = length(viewDir);
    viewDir /= distToEye;
    float3 normal = normalize(i.NormalW);
    
    float shadowFactor[MAXDIRLIGHTCOUNT];
    [unroll]
    for (int index = 0; index < MAXDIRLIGHTCOUNT; ++index)
    {
        shadowFactor[index] = 1;
    }
    float3 col = ComputeLighting(gLightCB, gMatCB, viewDir, normal, i.PosW, shadowFactor);
    col += gMatCB.Ambient;
    
    return float4(col, 1);
}

// 一条线变一个面，添加两个三角形
//    v2____v3
//     |\   |
//     | \  |
//     |  \ |
//     |___\|
//   v0(i0) v1(i1)
[maxvertexcount(6)]
void CylinderGS(line VertexPosWHNormalWColor input[2], inout TriangleStream<VertexPosWHNormalWColor> output)
{
    // 计算向上的单位向量
    float3 upDir = normalize(cross(input[0].NormalW, input[1].PosW - input[0].PosW));

    VertexPosWHNormalWColor vertexes[2];
    matrix viewProj = mul(gPassCB.View, gPassCB.Proj);
    [unroll]
    for (int i = 0; i < 2; i++) {
        vertexes[i].PosW = input[i].PosW + upDir * CylinderHeight;
        vertexes[i].NormalW = input[i].NormalW;
        vertexes[i].PosH = mul(float4(vertexes[i].PosW, 1), viewProj);
        vertexes[i].Color = input[i].Color;
    }

    output.Append(input[0]);
    output.Append(input[1]);
    output.Append(vertexes[0]);
    output.RestartStrip();

    output.Append(input[1]);
    output.Append(vertexes[1]);
    output.Append(vertexes[0]);
}

#endif