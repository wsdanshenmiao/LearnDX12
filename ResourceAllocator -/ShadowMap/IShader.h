#pragma once
#ifndef __SHADER__H__
#define __SHADER__H__

#include <memory>
#include "ShaderHelper.h"
#include "ConstantData.h"

namespace DSM {
    struct IShader
    {
    public:
        explicit IShader(ID3D12Device* device)
            :m_ShaderHelper(std::make_unique<ShaderHelper>()), m_Device(device){}
        virtual ~IShader() = default;
        
        virtual void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) = 0;
        
    protected:
        std::unique_ptr<ShaderHelper> m_ShaderHelper;
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
    };
}

#endif


