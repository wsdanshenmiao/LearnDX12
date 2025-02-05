#include "DSMCG.hlsli"
#include "DSMLighting.hlsli"

// SM 5.1 引进的新语法
ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);
ConstantBuffer<Lights> gLightCB : register(b2);
ConstantBuffer<MaterialConstants> gMatCB : register(b3);

VertexPosWHNormalW VS(VertexPosLNormalL v)
{
    VertexPosWHNormalW o;
    float4x4 viewProj = mul(gPassCB.View, gPassCB.Proj);
    float4 posW = mul(float4(v.PosL, 1), gObjCB.World);
    o.PosH = mul(posW, viewProj);
    o.PosW = posW.xyz;
    o.NormalW = mul(float4(v.NormalL, 1), gObjCB.WorldInvTranspos).xyz;
    return o;
}

float4 PS(VertexPosWHNormalW i) : SV_Target
{
    float3 viewDir = normalize(i.PosW - gPassCB.EyePosW);
    float3 normal = normalize(i.NormalW);
    
    float shadowFactor[MAXDIRLIGHTCOUNT];
    [unloop]
    for (int index = 0; index < MAXDIRLIGHTCOUNT; ++index)
    {
        shadowFactor[index] = 1;
    }
    float3 col = ComputeLighting(gLightCB, gMatCB, viewDir, normal, i.PosW, shadowFactor);
    col += gMatCB.Ambient;

    return float4(col, gMatCB.Alpha);
}