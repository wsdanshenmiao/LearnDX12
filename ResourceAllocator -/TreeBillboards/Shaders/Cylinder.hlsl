#ifndef __CYLINDER__HLSL__
#define __CYLINDER__HLSL__

#include "DSMCG.hlsli"

ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);
cbuffer gCylinderCB : register(b2)
{
    float CylinderHeight;
    float3 Pad;
}

VertexPosWHNormalWColor CylinderVS(VertexPosLNormalLColor i)
{
    VertexPosWHNormalWColor o;
    float4 posW = mul(float4(i.PosL, 1), gObjCB.World);
    o.PosH = mul(posW, mul(gPassCB.View, gPassCB.Proj));
    o.Color = i.Color;
    o.PosW = posW.xyz;
    o.NormalW = mul(i.NormalL, (float3x3)gObjCB.WorldInvTranspose);
    return o;
}

float4 CylinderPS(VertexPosWHNormalWColor i) : SV_Target
{
    return i.Color;
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