#include "DSMCG.hlsli"
#include "DSMLighting.hlsli"

// SM 5.1 引进的新语法
ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);
ConstantBuffer<Lights> gLightCB : register(b2);
ConstantBuffer<MaterialConstants> gMatCB : register(b3);

Texture2D gDiffuse : register(t0);
SamplerState gSamplerPointWrap          : register(s0);
SamplerState gSamplerLinearWrap         : register(s1);
SamplerState gSamplerAnisotropicWrap    : register(s2);
SamplerState gSamplerPointClamp         : register(s3);
SamplerState gSamplerLinearClamp        : register(s4);
SamplerState gSamplerAnisotropicClamp   : register(s5);

VertexPosWHNormalWTex VS(VertexPosLNormalLTex v)
{
    VertexPosWHNormalWTex o;
    float4x4 viewProj = mul(gPassCB.View, gPassCB.Proj);
    float4 posW = mul(float4(v.PosL, 1), gObjCB.World);
    o.PosH = mul(posW, viewProj);
    o.PosW = posW.xyz;
    o.NormalW = mul(float4(v.NormalL, 1), gObjCB.WorldInvTranspos).xyz;
    o.TexCoord = v.TexCoord;
    return o;
}

float4 PS(VertexPosWHNormalWTex i) : SV_Target
{
    float4 diffuseAlbedo = gDiffuse.Sample(gSamplerAnisotropicWrap, i.TexCoord);
    float alpha = diffuseAlbedo.a * gMatCB.Alpha;
#ifdef ALPHATEST
    // 进行alpha测试，剔除alpha过低的像素
    clip(alpha - 0.1f);
#endif
    
    float3 viewDir = i.PosW - gPassCB.EyePosW;
    float distToEye = length(viewDir);
    viewDir /= distToEye;
    float3 normal = normalize(i.NormalW);
    
    float shadowFactor[MAXDIRLIGHTCOUNT];
    [unloop]
    for (int index = 0; index < MAXDIRLIGHTCOUNT; ++index)
    {
        shadowFactor[index] = 1;
    }
    float3 col = ComputeLighting(gLightCB, gMatCB, viewDir, normal, i.PosW, shadowFactor);
    col += gMatCB.Ambient;

    float3 texCol = diffuseAlbedo.rgb;
    col *= texCol;

#ifdef ENABLEFOG
    float3 fogFactor = saturate((distToEye - gPassCB.FogStart) / gPassCB.FogRange);
    col = lerp(col, gPassCB.FogColor, fogFactor);
#endif

    return float4(col, alpha);
}