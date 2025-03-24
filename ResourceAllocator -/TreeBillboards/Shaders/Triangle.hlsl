#include "DSMLighting.hlsli"
#ifndef __TRIANGLE__HLSL__
#define __TRIANGLE__HLSL__

#include "DSMCG.hlsli"

ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);

VertexPosHColor TriangleVS(VertexPosLColor i)
{
    VertexPosHColor o;

    float4 posW = mul(float4(i.PosL, 1), gObjCB.World);
    matrix viewProj = mul(gPassCB.View, gPassCB.Proj);
    o.Color = i.Color;
    o.PosH = mul(posW, viewProj);

    return o;
}

float4 TrianglePS(VertexPosHColor i) : SV_Target
{
    return i.Color;
}


//       v1
//       /\
//      /  \
//   v3/____\v4
//    /\xxxx/\
//   /  \xx/  \
//  /____\/____\
// v0    v5    v2
[maxvertexcount(9)]
void TriangleGS(triangle VertexPosHColor input[3], inout TriangleStream<VertexPosHColor> output)
{
    VertexPosHColor vertexes[6];

    [unroll]
    for (int i = 0; i < 3; i++) {
        vertexes[i] = input[i];
        vertexes[i + 3].PosH = (input[i].PosH + input[(i + 1) % 3].PosH) * 0.5f;
        vertexes[i + 3].Color = (input[i].Color + input[(i + 1) % 3].Color) * 0.5f;
    }

    [unroll]
    for (int i = 0; i < 3; i++) {
        output.Append(vertexes[i]);
        output.Append(vertexes[i + 3]);
        output.Append(vertexes[(i + 2) % 3 + 3]);
        output.RestartStrip();
    }
}

#endif