#pragma once
#ifndef __SHADOWSHADER__H__
#define __SHADOWSHADER__H__

#include "ConstantData.h"
#include "FrameResource.h"
#include "IShader.h"

namespace DSM{
    class ShadowShader : public IShader
    {
    public:
        explicit ShadowShader(ID3D12Device* device);
        
        void SetObjectCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetPassCB(std::shared_ptr<D3D12ResourceLocation> cb);
        void SetMaterialCB(std::shared_ptr<D3D12ResourceLocation> cb);

        void SetObjectConstants(const ObjectConstants& objConstants);
        void SetPassConstants(const PassConstants& passConstants);
        void SetMaterialConstants(const MaterialConstants& materialConstants);
        void SetTexture(const D3D12DescriptorHandle& texture);

        void SetAlphaTest(bool enable);


        virtual void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) override;
        
    private:
        void CreateShader();


    private:
        bool m_EnableAlphaTest = true;
        std::array<std::shared_ptr<IShaderPass>, 2> m_ShaderPasses;
        std::uint8_t m_CurrentPass = 0;
    };

}


#endif