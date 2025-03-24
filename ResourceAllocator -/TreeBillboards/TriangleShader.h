#pragma once
#ifndef __TRIANGLESHADER__H__
#define __TRIANGLESHADER__H__

#include "IShader.h"

namespace DSM {
    class TriangleShader : public IShader
    {
    public:
        explicit TriangleShader(ID3D12Device* device);

        void SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetObjectConstants(const ObjectConstants& objConstants);
        void SetPassConstants(const PassConstants& passConstants);
        
        void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) override;

    private:
        void CreateShader();    
    };
}


#endif 

