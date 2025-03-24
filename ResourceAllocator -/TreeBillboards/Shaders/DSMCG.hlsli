#ifndef __DSMCG__HLSLI__
#define __DSMCG__HLSLI__

struct VertexPosLColor
{
    float3 PosL : POSITIONT;
    float4 Color : COLOR;
};

struct VertexPosLNormalL
{
    float3 PosL : POSITIONT;
    float3 NormalL : NORMAL;
};

struct VertexPosLNormalLTex
{
    float3 PosL : POSITIONT;
    float3 NormalL : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VertexPosHColor
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

struct VertexPosHColorTex
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

struct VertexPosWHNormalW
{
    float4 PosH : SV_Position;
    float3 NormalW : NORMAL;
    float3 PosW : TEXCOORD0;
};

struct VertexPosHNormalWTex
{
    float4 PosH : SV_Position;
    float3 NormalW : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VertexPosWHNormalWTex
{
    float4 PosH : SV_Position;
    float3 NormalW : NORMAL;
    float3 PosW : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct VertexPosLTex
{
    float4 PosL : POSITIONT;
    float2 TexCoord : TEXCOORD0;
};

struct VertexPosHTex
{
    float4 PosH : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VertexPosWHNormalWTexShadow
{
    float4 PosH : SV_Position;
    float3 NormalW : NORMAL;
    float3 PosW : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    float4 ShadowPosH : TEXCOORD2;
};

struct ObjectConstants
{
    float4x4 World;
    float4x4 WorldInvTranspose;
};

struct PassConstants
{
    float4x4 View;
    float4x4 InvView;
    float4x4 Proj;
    float4x4 InvProj;
    float4x4 ShadowTrans;
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
};






#endif