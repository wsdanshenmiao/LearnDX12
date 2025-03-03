#include "Color.hlsli"

VertexPosHColor VS(VertexPosLColor v)
{
    VertexPosHColor o;
    o.PosH = mul(float4(v.PosL, 1.0), gWorldViewProj);
    o.Color = v.Color;
    return o;
}

float4 PS(VertexPosHColor i) : SV_Target
{
    return i.Color;
}