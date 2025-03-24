#pragma once
#ifndef __SHADER__H__
#define __SHADER__H__

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
    };


    class CylinderShader : public IShader
    {
    public:
        explicit CylinderShader(ID3D12Device* device);
        
        void SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetCylinderCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetObjectConstants(const ObjectConstants& objConstants);
        void SetPassConstants(const PassConstants& passConstants);
        void SetCylinderConstants(float height);
        
        void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) override;
    };
}


#endif 

