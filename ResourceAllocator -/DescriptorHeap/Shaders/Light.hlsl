#include "DSMCG.hlsli"
#include "DSMLighting.hlsli"

// SM 5.1 引进的新语法
cbuffer ObjectConstants : register(b0)
{
    float4x4 World;
    float4x4 WorldInvTranspos;
}

cbuffer gPassCB : register(b1)
{
    float4x4 View;
    float4x4 InvView;
    float4x4 Proj;
    float4x4 InvProj;
    float3 EyePosW;
    float FogStart;
    float3 FogColor;
    float FogRange;
    float2 RenderTargetSize;
    float2 InvRenderTargetSize;
    float NearZ;
    float FarZ;
    float TotalTime;
    float DeltaTime;
}

cbuffer Lights : register(b2)
{
    DirectionalLight DirectionalLights[MAXDIRLIGHTCOUNT];
    PointLight PointLights[MAXPOINTLIGHTCOUNT];
    SpotLight SpotLights[MAXSPOTLIGHTCOUNT];
}

cbuffer MaterialConstants : register(b3)
{
    float3 Diffuse;
    float Alpha;
    float3 Specular;
    float Gloss;
    float3 Ambient;
    float pad0;
}

float4 gCol;

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
    float4x4 viewProj = mul(View, Proj);
    float4 posW = mul(float4(v.PosL, 1), World);
    o.PosH = mul(posW, viewProj);
    o.PosW = posW.xyz;
    o.NormalW = mul(float4(v.NormalL, 1), WorldInvTranspos).xyz;
    o.TexCoord = v.TexCoord;
    return o;
}

float4 PS(VertexPosWHNormalWTex i) : SV_Target
{
    float4 diffuseAlbedo = gDiffuse.Sample(gSamplerAnisotropicWrap, i.TexCoord);
    float alpha = diffuseAlbedo.a * Alpha;
#ifdef ALPHATEST
    // 进行alpha测试，剔除alpha过低的像素
    clip(alpha - 0.1f);
#endif
    
    float3 viewDir = i.PosW - EyePosW;
    float distToEye = length(viewDir);
    viewDir /= distToEye;
    float3 normal = normalize(i.NormalW);
    
    float shadowFactor[MAXDIRLIGHTCOUNT];
    [unroll]
    for (int index = 0; index < MAXDIRLIGHTCOUNT; ++index)
    {
        shadowFactor[index] = 1;
    }

    Lights lights;
    lights.DirectionalLights = DirectionalLights;
    lights.PointLights = PointLights;
    lights.SpotLights = SpotLights;

    MaterialConstants mat;
    mat.Diffuse = Diffuse;
    mat.Alpha = Alpha;
    mat.Specular = Specular;
    mat.Gloss = Gloss;
    mat.Ambient = Ambient;
    
    float3 col = ComputeLighting(lights, mat, viewDir, normal, i.PosW, shadowFactor);
    col += Ambient;

    float3 texCol = diffuseAlbedo.rgb;
    col *= texCol;

    float4 lCol = gCol;
    lCol.rgb = col;
    texCol = lCol.rgb;

#ifdef ENABLEFOG
    float3 fogFactor = saturate((distToEye - FogStart) / FogRange);
    col = lerp(col, FogColor, fogFactor);
#endif

    return float4(col, alpha);
}