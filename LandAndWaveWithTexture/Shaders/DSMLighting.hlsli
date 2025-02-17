#ifndef __DSMLIGHTING__HLSLI__
#define __DSMLIGHTING__HLSLI__


// 需要使用全局宏定义最大光源的数量
#ifndef MAXDIRLIGHTCOUNT
#define MAXDIRLIGHTCOUNT 5
#endif

#ifndef MAXPOINTLIGHTCOUNT
#define MAXPOINTLIGHTCOUNT 5
#endif

#ifndef MAXSPOTLIGHTCOUNT
#define MAXSPOTLIGHTCOUNT 5
#endif

struct DirectionalLight
{
    float3 Color;
    float pad0;
    float3 Dir;
    float pad1;
};

struct PointLight
{
    float3 Color;
    float StartAtten;
    float3 Pos;
    float EndAtten;
};

struct SpotLight
{
    float3 Color;
    float StartAtten;
    float3 Pos;
    float EndAtten;
    float3 Dir;
    float SpotPower;
};

struct MaterialConstants
{
    float3 Diffuse;
    float Alpha;
    float3 Specular;
    float Gloss;
    float3 Ambient;
    float pad0;
};

// 各种光源的集合，需要按顺序传递光源
struct Lights
{
    DirectionalLight DirectionalLights[MAXDIRLIGHTCOUNT];
    PointLight PointLights[MAXPOINTLIGHTCOUNT];
    SpotLight SpotLights[MAXSPOTLIGHTCOUNT];
};

float CalcuAttenuation(float distance, float startAtten, float endAtten)
{
    return saturate((endAtten - distance) / (endAtten - startAtten));
}

float3 BlinnPhong(MaterialConstants mat, float3 lightColor, float3 lightDir, float3 viewDir, float3 normal)
{
    const float m = mat.Gloss * 256;
    float3 halfDir = normalize(lightDir + viewDir);
    
    float3 diffuse = lightColor * mat.Diffuse * max(0, dot(lightDir, normal));
    float3 specular = lightColor * mat.Specular * pow(max(0, dot(lightDir, halfDir)), m);
    
    return diffuse + specular;
}

float3 ComputeDirectionalLight(DirectionalLight light, MaterialConstants mat, float3 viewDir, float3 normal)
{
    float3 lightDir = -light.Dir;
    normalize(lightDir);
    return BlinnPhong(mat, light.Color, lightDir, viewDir, normal);
}

float3 ComputePointLight(PointLight light, MaterialConstants mat, float3 viewDir, float3 normal, float3 posW)
{
    float3 lightDir = light.Pos - posW;
    float distance = length(lightDir);
    if (distance > light.EndAtten)
        return 0;
    
    lightDir /= distance;
    float atten = CalcuAttenuation(distance, light.StartAtten, light.EndAtten);
    float3 lightColor = light.Color * atten;

    return BlinnPhong(mat, lightColor, lightDir, viewDir, normal);
}

float3 ComputeSpotLight(SpotLight light, MaterialConstants mat, float3 viewDir, float3 normal, float3 posW)
{
    float3 lightDir = light.Pos - posW;
    float distance = length(lightDir);
    if (distance > light.EndAtten)
        return 0;
    
    lightDir /= distance;
    float atten = CalcuAttenuation(distance, light.StartAtten, light.EndAtten);
    atten *= pow(max(0, dot(normalize(light.Dir), -lightDir)), light.SpotPower);
    float3 lightColor = light.Color * atten;
    
    return BlinnPhong(mat, lightColor, lightDir, viewDir, normal);
}

// 需要传入归一化后的viewDir和normal
float3 ComputeLighting(
    Lights lights,
    MaterialConstants mat,
    float3 viewDir,
    float3 normal,
    float3 posW,
    float shadowFactor[MAXDIRLIGHTCOUNT])
{
    float3 col = 0;
    
    [unroll]
    for (uint i = 0; i < MAXDIRLIGHTCOUNT; ++i)
    {
        // 仅直接光接收阴影
        col += ComputeDirectionalLight(lights.DirectionalLights[i], mat, viewDir, normal) * shadowFactor[i];
    }
    [unroll]
    for (uint i = 0; i < MAXPOINTLIGHTCOUNT; ++i)
    {
        col += ComputePointLight(lights.PointLights[i], mat, viewDir, normal, posW);
    }
    [unroll]
    for (uint i = 0; i < MAXSPOTLIGHTCOUNT; ++i)
    {
        col += ComputeSpotLight(lights.SpotLights[i], mat, viewDir, normal, posW);
    }

    return col;
}

#endif