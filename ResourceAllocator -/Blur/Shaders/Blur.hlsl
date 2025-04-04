#ifndef __BLUR__HLSL__
#define __BLUR__HLSL__

// 一个线程组中有 256 个线程
#define THREAD_SIZE 256

// 最大模糊半径
static const int gMaxBlurRadius = 5;

cbuffer BlurConstants : register(b0)
{
    int gBlurRadius;
    float gBlurWeights[2 * gMaxBlurRadius];
}

Texture2D gBlurTexture : register(t0);
RWTexture2D<float4> gOutput : register(t1);

// 使用共享内存缓存采样的结果，避免重复的采样
groupshared float4 gCache[THREAD_SIZE + 2 * gMaxBlurRadius];

[numthreads(THREAD_SIZE, 1, 1)]
void HorizBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    // 由于边缘的像素也需要模糊，因此在共享内存中对超出范围的采样结果进行钳值
    if (groupThreadID.x < gMaxBlurRadius) {    // 左侧的钳值
        // 若纹理的大小不能被线程数整除，会有多余的线程，需要进行索引限制
        int x = max(0, groupThreadID.x - gBlurRadius);
        gCache[groupThreadID.x] = gBlurTexture[x, dispatchThreadID.y];
    }
    if (groupThreadID.x >= THREAD_SIZE - gBlurRadius) { // 右侧钳值
        int x = min(gBlurTexture.Length.x - 1, groupThreadID.x + gBlurRadius);
        gCache[2 * gBlurRadius + groupThreadID.x] = gBlurTexture[x, dispatchThreadID.y];
    }

    // 当前线程负责的采样
    int2 uv = min(groupThreadID.xy, gBlurTexture.Length.xy - 1);
    gCache[groupThreadID.x + gBlurRadius] = gBlurTexture[uv];

    // 等待所有的线程采样完成
    GroupMemoryBarrierWithGroupSync();

    // 进行模糊
    float4 col = 0;
    for (int i = -gBlurRadius; i <= gBlurRadius; i++) {
        int index = i + gBlurRadius;
        col += gBlurWeights[index] * gCache[index + groupThreadID.x];
    }

    gOutput[dispatchThreadID.xy] = col;
}

[numthreads(THREAD_SIZE, 1, 1)]
void VerticBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    // 由于边缘的像素也需要模糊，因此在共享内存中对超出范围的采样结果进行钳值
    if (groupThreadID.y < gMaxBlurRadius) {    // 左侧的钳值
        // 若纹理的大小不能被线程数整除，会有多余的线程，需要进行索引限制
        int y = max(0, groupThreadID.y - gBlurRadius);
        gCache[groupThreadID.y] = gBlurTexture[dispatchThreadID.x, y];
    }
    if (groupThreadID.y >= THREAD_SIZE - gBlurRadius) { // 右侧钳值
        int y = min(gBlurTexture.Length.y - 1, groupThreadID.y + gBlurRadius);
        gCache[2 * gBlurRadius + groupThreadID.y] = gBlurTexture[dispatchThreadID.x, y];
    }

    // 当前线程负责的采样
    int2 uv = min(groupThreadID.xy, gBlurTexture.Length.xy - 1);
    gCache[groupThreadID.y + gBlurRadius] = gBlurTexture[uv];

    // 等待所有的线程采样完成
    GroupMemoryBarrierWithGroupSync();

    // 进行模糊
    float4 col = 0;
    for (int i = -gBlurRadius; i <= gBlurRadius; i++) {
        int index = i + gBlurRadius;
        col += gBlurWeights[index] * gCache[index + groupThreadID.y];
    }

    gOutput[dispatchThreadID.xy] = col;
}

#endif