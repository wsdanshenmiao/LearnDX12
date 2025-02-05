#include "Color.hlsli"

VertexPosHColor VS(VertexPosLColor v)
{
    VertexPosHColor o;
    o.PosH = float4(v.PosL, 1);
    o.Color = v.Color;
    return o;
}

float4 PS(VertexPosHColor i) : SV_Target
{
    return i.Color;
}