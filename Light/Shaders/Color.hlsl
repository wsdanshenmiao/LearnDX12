#include "DSMCG.hlsli"

// 常量缓冲区的大小必须为 256 b 的整数倍，隐式填充为 256 byte
// SM 5.1 引进的新语法
ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);

VertexPosHColor VS(VertexPosLColor v)
{
    VertexPosHColor o;
    float4x4 viewProj = mul(gPassCB.View, gPassCB.Proj);
    float4 posW = mul(float4(v.PosL, 1), gObjCB.World);
    o.PosH = mul(posW, viewProj);
    o.Color = v.Color;
    return o;
}

float4 PS(VertexPosHColor i) : SV_Target
{
    return i.Color;
}