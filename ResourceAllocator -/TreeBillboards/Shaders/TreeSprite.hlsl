#include "DSMCG.hlsli"
#include "DSMLighting.hlsli"

ConstantBuffer<ObjectConstants> gObjCB : register(b0);
ConstantBuffer<PassConstants> gPassCB : register(b1);
ConstantBuffer<Lights> gLightCB : register(b2);
ConstantBuffer<MaterialConstants> gMatCB : register(b3);

Texture2DArray gTreeTexArray : register(t0);

SamplerState gSamplerAnisotropicWrap    : register(s2);

struct Attributes
{
    float3 PosL : POSITION;
    float2 Size : SIZE;
};

struct Varyings
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoord : TEXCOORD0;
    uint PrimID : SV_PrimitiveID;   // 由于使用了GS，PS要使用图元ID需要手动传递
};

Attributes VS(Attributes i)
{
    Attributes o;
    o.PosL = i.PosL;
    o.Size = i.Size;
    return o;
}

[maxvertexcount(4)]
void GS(point Attributes input[1], uint primID : SV_PrimitiveID, inout TriangleStream<Varyings> output)
{
    float3 posW = mul(float4(input[0].PosL, 1), gObjCB.World).xyz;
    float3 upDir = float3(0, 1, 0);
    float3 viewDir = gPassCB.EyePosW - posW;
    viewDir.y = 0;  // 让面片一直树立着
    viewDir = normalize(viewDir);
    float3 rightDir = cross(upDir, viewDir);

    // 将一个点扩增为四个点
    /*  v3      v1
     *  |-------|
     *  |       |
     *  |       |
     *  |-------|
     *  v2      v0
     */
    float2 halfSize = input[0].Size * 0.5f;
    float4 vertexes[4];
    vertexes[0] = float4(posW + rightDir * halfSize.x - upDir * halfSize.y, 1);
    vertexes[1] = float4(posW + rightDir * halfSize.x + upDir * halfSize.y, 1);
    vertexes[2] = float4(posW - rightDir * halfSize.x - upDir * halfSize.y, 1);
    vertexes[3] = float4(posW - rightDir * halfSize.x + upDir * halfSize.y, 1);

    float2 texCoords[4] = {
        float2(0, 1), float2(0, 0),
        float2(1, 1), float2(1, 0)};

    Varyings o;
    matrix viewProj =  mul(gPassCB.View, gPassCB.Proj);
    [unroll]
    for (int i = 0; i < 4; ++i) {
        o.PosW = vertexes[i].xyz;
        o.PosH = mul(vertexes[i], viewProj);
        o.NormalW = viewDir;
        o.TexCoord = texCoords[i];
        o.PrimID = primID;
        output.Append(o);   // 输出为三角形带
    }
}

float4 PS(Varyings i) : SV_Target
{
    float3 uvw = float3(i.TexCoord, i.PrimID % 3);  // 使用纹理数组时z为索引
    float4 diffuseAlbedo = gTreeTexArray.Sample(gSamplerAnisotropicWrap, uvw);
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
    [unroll]
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