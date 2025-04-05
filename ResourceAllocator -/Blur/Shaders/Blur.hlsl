#ifndef __BLUR__HLSL__
#define __BLUR__HLSL__

// 一个线程组中有 256 个线程
#define THREAD_SIZE 256

// 最大模糊半径
static const int gMaxBlurRadius = 5;

struct BlurWeights
{
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

cbuffer BlurConstants : register(b0)
{
    int gBlurRadius;
    float3 pads0;
    BlurWeights gBlurWeights;
}

Texture2D gBlurTexture : register(t0);
RWTexture2D<float4> gOutput : register(u0);

// 使用共享内存缓存采样的结果，避免重复的采样
groupshared float4 gCache[THREAD_SIZE + 2 * gMaxBlurRadius];

[numthreads(THREAD_SIZE, 1, 1)]
void HorizBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { gBlurWeights.w0, gBlurWeights.w1, gBlurWeights.w2,
        gBlurWeights.w3,gBlurWeights.w4, gBlurWeights.w5, gBlurWeights.w6,
        gBlurWeights.w7,gBlurWeights.w8, gBlurWeights.w9, gBlurWeights.w10 };
    int width, height;
    gBlurTexture.GetDimensions(width, height);
    
    // 由于边缘的像素也需要模糊，因此在共享内存中对超出范围的采样结果进行钳值
    if (groupThreadID.x < gBlurRadius) {    // 左侧的钳值
        // 若纹理的大小不能被线程数整除，会有多余的线程，需要进行索引限制
        int x = max(0, dispatchThreadID.x - gBlurRadius);
        gCache[groupThreadID.x] = gBlurTexture[int2(x, dispatchThreadID.y)];
    }
    if (groupThreadID.x >= THREAD_SIZE - gBlurRadius) { // 右侧钳值
        int x = min(width - 1, dispatchThreadID.x + gBlurRadius);
        gCache[2 * gBlurRadius + groupThreadID.x] = gBlurTexture[int2(x, dispatchThreadID.y)];
    }

    // 当前线程负责的采样
    int2 uv = min(dispatchThreadID.xy, int2(width,height) - 1);
    gCache[groupThreadID.x + gBlurRadius] = gBlurTexture[uv];

    // 等待所有的线程采样完成
    GroupMemoryBarrierWithGroupSync();

    // 进行模糊
    float4 col = 0;
    for (int iii = -gBlurRadius; iii <= gBlurRadius; ++iii) {
        int index = iii + gBlurRadius;
        col += weights[index] * gCache[index + groupThreadID.x];
    }

    gOutput[dispatchThreadID.xy] = col;
}

[numthreads(1, THREAD_SIZE, 1)]
void VerticBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { gBlurWeights.w0, gBlurWeights.w1, gBlurWeights.w2,
        gBlurWeights.w3,gBlurWeights.w4, gBlurWeights.w5, gBlurWeights.w6,
        gBlurWeights.w7,gBlurWeights.w8, gBlurWeights.w9, gBlurWeights.w10 };
    int width, height;
    gBlurTexture.GetDimensions(width, height);
    
    // 由于边缘的像素也需要模糊，因此在共享内存中对超出范围的采样结果进行钳值
    if (groupThreadID.y < gBlurRadius) {    // 左侧的钳值
        // 若纹理的大小不能被线程数整除，会有多余的线程，需要进行索引限制
        int y = max(0, dispatchThreadID.y - gBlurRadius);
        gCache[groupThreadID.y] = gBlurTexture[int2(dispatchThreadID.x, y)];
    }
    if (groupThreadID.y >= THREAD_SIZE - gBlurRadius) { // 右侧钳值
        int y = min(width - 1, dispatchThreadID.y + gBlurRadius);
        gCache[2 * gBlurRadius + groupThreadID.y] = gBlurTexture[int2(dispatchThreadID.x, y)];
    }

    // 当前线程负责的采样
    int2 uv = min(dispatchThreadID.xy, int2(width,height) - 1);
    gCache[groupThreadID.y + gBlurRadius] = gBlurTexture[uv];

    // 等待所有的线程采样完成
    GroupMemoryBarrierWithGroupSync();

    // 进行模糊
    float4 col = 0;
    for (int iii = -gBlurRadius; iii <= gBlurRadius; ++iii) {
        int index = iii + gBlurRadius;
        col += weights[index] * gCache[index + groupThreadID.y];
    }

    gOutput[dispatchThreadID.xy] = col;
}

#endif