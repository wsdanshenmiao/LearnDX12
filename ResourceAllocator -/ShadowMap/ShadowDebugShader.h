#pragma once
#ifndef __SHADOWDEBUGSHADER__H__
#define __SHADOWDEBUGSHADER__H__

#include "IShader.h"

namespace DSM {
    class ShadowDebugShader : public IShader
    {
    public:
        explicit ShadowDebugShader(ID3D12Device* device);
        void SetTexture(const D3D12DescriptorHandle& handle);
        void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) override;

    private:
        void CreateShader();
    };
}

#endif

