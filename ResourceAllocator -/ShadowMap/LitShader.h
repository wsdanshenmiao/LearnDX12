#pragma once
#ifndef __LIGHTSHADER__H__
#define __LIGHTSHADER__H__

#include "IShader.h"

namespace DSM{
    class LitShader : public IShader
    {
    public:
        LitShader(ID3D12Device* device,
            std::uint32_t numDirLight = 3,
            std::uint32_t numPointLight = 1,
            std::uint32_t numSpotLight = 1);

        void SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetMaterialCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetLightCB(std::shared_ptr<D3D12ResourceLocation> cb);
        
        void SetObjectConstants(const ObjectConstants& objConstants);
        void SetPassConstants(const PassConstants& passConstants);
        void SetMaterialConstants(const MaterialConstants& materialConstants);
        void SetDirectionalLights(std::size_t byteSize, const void* lightData);
        void SetPointLights(std::size_t byteSize, const void* lightData);
        void SetSpotLights(std::size_t byteSize, const void* lightData);
        void SetTexture(const D3D12DescriptorHandle& texture);
        void SetShadowMap(const D3D12DescriptorHandle& shadowMap);
        
        virtual void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) override;
    };


    
}


#endif

