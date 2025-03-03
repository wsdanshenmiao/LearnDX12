
struct VertexPosLColor
{
    float3 PosL : POSITIONT;
    float4 Color : COLOR;
};

struct VertexPosHColor
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
};

struct VertexPosLNormalColor
{
    float3 PosL : POSITIONT;
    float3 NormalV : NORMAL;
    float4 Color : COLOR;
};

// 常量缓冲区的大小必须为 256 b 的整数倍，隐式填充为 256 byte
cbuffer CBPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspos;
}

cbuffer CBPerPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float3 gEyePosW;
    float gcbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
}

//// SM 5.1 引进的新语法
//struct ObjectConstants
//{
//    float4x4 gWorldViewProj;
//};
//ConstantBuffer<ObjectConstants> gObjConstants : register(b0);

