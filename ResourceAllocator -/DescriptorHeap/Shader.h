#pragma once
#ifndef __SHADER__H__
#define __SHADER__H__

#include "D3D12Resource.h"
#include "ShaderProperty.h"

namespace DSM {

    enum class ShaderType : int
    {
        VERTEX_SHADER,
        HUL_SHADER,
        DOMAIN_SHADER,
        GEOMETRY_SHADER,
        PIXEL_SHADER,
        COMPUTE_SHADER,
        NUM_SHADER_TYPES
    };

    class ShaderDefines
    {
    public:
        void AddDefine(const std::string& name, const std::string& value);
        void RemoveDefine(const std::string& name);
        std::vector<D3D_SHADER_MACRO> GetShaderDefines() const;

    private:
        std::map<std::string, std::string> m_Defines;
    };
    
    struct ShaderPassEesc
    {
        std::string m_VSName;
        std::string m_HSName;
        std::string m_DSName;
        std::string m_GSName;
        std::string m_PSName;
        std::string m_CSName;
    };

    struct ShaderDesc
    {
        std::string m_ShaderName;
        std::string m_FileName;
        std::string m_EnterPoint;
        std::string m_OutputFileName;
        std::string m_Target;
        ShaderType m_Type;
        ShaderDefines m_Defines;
    };


    struct IShaderPass
    {
        virtual void SetBlendState(const D3D12_BLEND_DESC& blendDesc) = 0;
        virtual void SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc) = 0;
        virtual void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc) = 0;
        
    };

    struct IConstantBufferVariable
    {
        virtual void SetInt(int val) = 0;
        virtual void SetFloat(float val) = 0;
        virtual void SetVector(const DirectX::XMFLOAT4& val) = 0;
        virtual void SetMatrix(const DirectX::XMFLOAT4X4& val) = 0;
        virtual void SetRow(std::uint32_t byteSize, const void* data, std::uint32_t offset = 0) = 0;
    
        virtual ~IConstantBufferVariable() = default;
    };

    // 用于管理所有的 Shader
    class ShaderHealper
    {
    public:
        ShaderHealper();
        ~ShaderHealper();
        ShaderHealper(const ShaderHealper&) = delete;
        ShaderHealper& operator=(const ShaderHealper&) = delete;
        
        void SetFrameCount(std::uint32_t frameCount);
        void CreateShaderFormFile(const ShaderDesc& shaderDesc);
    
        
    public:
        inline static std::uint32_t FrameCount = 3;
        
    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };
    
    class Shader
    {
    public:

    private:
        // ShaderPass ，可指定使用哪些 Shader
        std::unordered_map<std::string, IShaderPass> m_ShaderPasses;
    };
    
}

#endif