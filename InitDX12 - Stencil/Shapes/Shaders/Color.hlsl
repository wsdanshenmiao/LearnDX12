#include "Color.hlsli"

VertexPosHColor VS(VertexPosLColor v)
{
    VertexPosHColor o;
    float4x4 viewProj = mul(gView, gProj);
    float4 posW = mul(float4(v.PosL, 1), gWorld);
    o.PosH = mul(posW, viewProj);
    o.Color = v.Color;
    return o;
}

float4 PS(VertexPosHColor i) : SV_Target
{
    return i.Color;
}