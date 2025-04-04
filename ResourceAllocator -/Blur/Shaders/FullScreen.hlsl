#ifndef __FULLSCREEN__HLSL__
#define __FULLSCREEN__HLSL__

// 使用一个三角形覆盖NDC空间 
// (-1, 1)________ (3, 1)
//        |   |  /
// (-1,-1)|___|/ (1, -1)   
//        |  /
// (-1,-3)|/    
float4 FullScreenVS(uint vertexID : SV_VertexID) : SV_Position
{
    float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
    float2 xy = grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    return float4(xy, 1.0f, 1.0f);
}


#endif