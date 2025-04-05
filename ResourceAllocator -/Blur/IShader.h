#pragma once
#ifndef __ISHADER__H__
#define __ISHADER__H__

#include <memory>
#include "ShaderHelper.h"
#include "ConstantData.h"

namespace DSM {
    struct IShader
    {
    public:
        explicit IShader()
            :m_ShaderHelper(std::make_unique<ShaderHelper>()){}
        virtual ~IShader() = default;
        
        virtual void Apply(ID3D12GraphicsCommandList* cmdList, FrameResource* frameResource) = 0;
        
    protected:
        std::unique_ptr<ShaderHelper> m_ShaderHelper;
    };
}

#endif


