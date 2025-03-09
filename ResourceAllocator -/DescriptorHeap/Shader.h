#pragma once
#ifndef __SHADER__H__
#define __SHADER__H__

#include <d3d12.h>
#include <string>
#include <vector>
#include <map>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>
#include <wrl/client.h>
#include <dxcapi.h>

#include "D3D12DescriptorHeap.h"
#include "D3D12Resource.h"

namespace DSM {
    class FrameResource;
    

    enum class ShaderType : int
    {
        VERTEX_SHADER,
        HULL_SHADER,
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
        std::vector<DxcDefine> GetShaderDefinesDxc() const;

    private:
        std::map<std::string, std::string> m_Defines;
        std::map<std::wstring, std::wstring> m_DefinesDxc;
    };
    
    struct ShaderPassDesc
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

    struct IConstantBufferVariable
    {
        virtual void SetInt(int val) = 0;
        virtual void SetFloat(float val) = 0;
        virtual void SetVector(const DirectX::XMFLOAT4& val) = 0;
        virtual void SetMatrix(const DirectX::XMFLOAT4X4& val) = 0;
        virtual void SetRow(std::uint32_t byteSize, const void* data, std::uint32_t offset = 0) = 0;
    
        virtual ~IConstantBufferVariable() = default;
    };

    class ShaderHelper;
    struct IShaderPass
    {
        virtual void SetBlendState(const D3D12_BLEND_DESC& blendDesc) = 0;
        virtual void SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc) = 0;
        virtual void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc) = 0;
        virtual void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout) = 0;
        virtual void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) = 0;
        virtual void SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc) = 0;
        virtual void SetSampleDesc(std::uint32_t count, std::uint32_t quality) = 0;
        virtual void SetDSVFormat(DXGI_FORMAT format) = 0;
        virtual void SetRTVFormat(const std::vector<DXGI_FORMAT>& formats) = 0;

        // 获取顶点着色器的uniform形参用于设置值
        virtual std::shared_ptr<IConstantBufferVariable> VSGetParamByName(const std::string& paramName) = 0;
        virtual std::shared_ptr<IConstantBufferVariable> DSGetParamByName(const std::string& paramName) = 0;
        virtual std::shared_ptr<IConstantBufferVariable> HSGetParamByName(const std::string& paramName) = 0;
        virtual std::shared_ptr<IConstantBufferVariable> GSGetParamByName(const std::string& paramName) = 0;
        virtual std::shared_ptr<IConstantBufferVariable> PSGetParamByName(const std::string& paramName) = 0;
        virtual std::shared_ptr<IConstantBufferVariable> CSGetParamByName(const std::string& paramName) = 0;
        
        virtual const ShaderHelper* GetShaderHelper() const = 0;
        virtual const std::string& GetPassName()const = 0;
        virtual void Dispatch(
            ID3D12GraphicsCommandList* cmdList,
            std::uint32_t threadX = 1,
            std::uint32_t threadY = 1,
            std::uint32_t threadZ = 1) = 0;
        virtual void Apply(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device, FrameResource* frameResourc) = 0;
        virtual ~IShaderPass() = default;
    };


    // 用于管理所有的 Shader
    class ShaderHelper
    {
    public:
        ShaderHelper();
        ~ShaderHelper();
        ShaderHelper(const ShaderHelper&) = delete;
        ShaderHelper& operator=(const ShaderHelper&) = delete;
        
        void SetFrameCount(std::uint32_t frameCount);

        void SetConstantBufferByName(const std::string& name, std::shared_ptr<D3D12ResourceLocation> cb);
        void SetConstantBufferBySlot(std::uint32_t slot, std::shared_ptr<D3D12ResourceLocation> cb);
        void SetShaderResourceByName(const std::string& name, const std::vector<D3D12DescriptorHandle>& resource);
        void SetShaderResourceBySlot(std::uint32_t slot, const std::vector<D3D12DescriptorHandle>& resource);
        void SetRWResourceByName(const std::string& name, const std::vector<D3D12DescriptorHandle>& resource);
        void SetRWResrouceBySlot(std::uint32_t slot, const std::vector<D3D12DescriptorHandle>& resource);
        void SetSampleStateByName(const std::string& name, const std::vector<D3D12DescriptorHandle>& sampleState);
        void SetSampleStateBySlot(std::uint32_t slot, const std::vector<D3D12DescriptorHandle>& sampleState);
        
        std::shared_ptr<IConstantBufferVariable> GetConstantBufferVariable(const std::string& name);
        
        void AddShaderPass(const std::string& shaderPassName, const ShaderPassDesc& passDesc, ID3D12Device* device);
        std::shared_ptr<IShaderPass> GetShaderPass(const std::string& passName);
        
        void CreateShaderFormFile(const ShaderDesc& shaderDesc);
        void Clear();

    private:
        Microsoft::WRL::ComPtr<ID3DBlob> DXCCreateShaderFromFile(const ShaderDesc& shaderDesc);
        Microsoft::WRL::ComPtr<ID3DBlob> D3DCompileCreateShaderFromFile(const ShaderDesc& shaderDesc);
        
        
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