#include "Shader.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace DSM {
    
    struct ShaderParameter
    {
        std::string m_Name;
        std::uint32_t m_BindPoint;
        std::uint32_t m_RegisterSpace;
        std::shared_ptr<D3D12ResourceLocation> m_Resource;
    };


    // 着色器资源
    struct ShaderResource : ShaderParameter
    {
        D3D12_SRV_DIMENSION m_Dimension;
    };

    // 可读写资源
    struct RWResource : ShaderParameter
    {
        D3D12_UAV_DIMENSION m_Dimension;
        uint32_t m_InitialCount;
        bool m_EnableCounter;
        bool m_FirstInit;
    };

    struct SamplerState : ShaderParameter{};

    // 常量缓冲区
    struct ConstantBuffer : ShaderParameter
    {
        std::uint32_t m_NumFrameDirty;
        std::vector<std::uint8_t> m_Data;

        // 更新常量缓冲区
        void UpdateBuffer()
        {
            if (m_NumFrameDirty > 0) {
                memcpy(m_Resource->m_MappedBaseAddress, m_Data.data(), m_Data.size());
                m_NumFrameDirty--;
            }
        }
    };

    struct ConstantBufferVariable : IConstantBufferVariable
    {
        void SetInt(int val) override
        {
            SetRow(sizeof(int), &val);
        }
        
        void SetFloat(float val) override
        {
            SetRow(sizeof(float), &val);
        }
        
        void SetVector(const DirectX::XMFLOAT4& val) override
        {
            SetRow(sizeof(XMFLOAT4), &val);
        }

        void SetMatrix(const DirectX::XMFLOAT4X4& val) override
        {
            SetRow(sizeof(XMFLOAT4X4), &val);
        }

        void SetRow(std::uint32_t byteSize, const void* data, std::uint32_t offset = 0) override
        {
            assert(data != nullptr);
            byteSize = min(byteSize, m_ByteSize);
            memcpy(m_ConstantBuffer->m_Data.data() + m_StartOffset, data, byteSize);

            m_ConstantBuffer->m_NumFrameDirty = ShaderHealper::FrameCount;
        }

        std::string m_Name;
        std::uint32_t m_StartOffset;
        std::uint32_t m_ByteSize;
        ConstantBuffer* m_ConstantBuffer;
    };
    

    struct ShaderInfo
    {
        std::string m_Name;
        ComPtr<ID3DBlob> m_pShader;
        std::uint32_t m_CbUseMask = 0;
        std::uint32_t m_SsUseMask = 0;
        std::uint32_t m_SrUseMasks[4] = {};
        std::unique_ptr<ConstantBuffer> m_pParamData = nullptr; // 每个着色器有自己的参数常量缓冲区
        std::map<std::string, std::shared_ptr<ConstantBufferVariable>> m_ConstantBufferVariable;    // 常量缓冲区区变量 
        virtual ~ShaderInfo() = default;
    };

    struct PixelShaderInfo : ShaderInfo
    {
        std::uint32_t m_RwUseMask = 0;
    };

    struct ComputeShaderInfo : ShaderInfo
    {
        std::uint32_t m_RwUseMask = 0;
        std::uint32_t m_ThreadGroupSizeX = 0;
        std::uint32_t m_ThreadGroupSizeY = 0;
        std::uint32_t m_ThreadGroupSizeZ = 0;
    };
    

    //
    // ShaderDefines Implementation
    //
    void ShaderDefines::AddDefine(const std::string& name, const std::string& value)
    {
        m_Defines[name] = value;
    }

    void ShaderDefines::RemoveDefine(const std::string& name)
    {
        if (m_Defines.contains(name)) {
            m_Defines.erase(name);
        }
    }

    std::vector<D3D_SHADER_MACRO> ShaderDefines::GetShaderDefines() const
    {
        std::vector<D3D_SHADER_MACRO> defines(m_Defines.size() + 1);
        for (const auto& define : m_Defines) {
            defines.push_back({ define.first.c_str(), define.second.c_str() });
        }
        defines.push_back({ nullptr, nullptr });
        return defines;
    }



    
    //
    // ShaderHealper Implementation
    //
    struct ShaderHealper::Impl
    {
        ~Impl() = default;
        
        void GetShaderInfo(std::string name, ShaderType shaderType, ID3DBlob* shaderByteCode);
        
        
        // 存储编译后的 Shader代码
        std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_ShaderPassByteCode;
        // 着色器的信息
        std::unordered_map<std::string, std::shared_ptr<ShaderInfo>> m_ShaderInfo;

        // 各种着色器资源，需要所有着色器的常量缓冲区没有冲突
        std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> m_ConstantBufferVariables;
        std::unordered_map<std::uint32_t, ConstantBuffer> m_ConstantBuffers;
        std::unordered_map<std::uint32_t, ShaderResource> m_ShaderResources;
        std::unordered_map<std::uint32_t, RWResource> m_RWResources;
        std::unordered_map<std::uint32_t, SamplerState> m_SamplerStates;
        
    private:
        void GetConstantBufferInfo(
            ID3D12ShaderReflection* shaderReflection,
            const D3D12_SHADER_INPUT_BIND_DESC& bindDes,
            ShaderType shaderType,
            const std::string& name);
        void GetShaderResourceInfo(
            const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
            ShaderType shaderType,
            const std::string& name);
        void GetRWResourceInfo(
            const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
            ShaderType shaderType,
            const std::string& name);
        void GetSamplerStateInfo(
            const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
            ShaderType shaderType,
            const std::string& name);
    };

    void ShaderHealper::Impl::GetShaderInfo(std::string name, ShaderType shaderType, ID3DBlob* shaderByteCode)
    {
        // 获取着色器反射
        ComPtr<ID3D12ShaderReflection> shaderReflection;
        ThrowIfFailed(D3DReflect(shaderByteCode->GetBufferPointer(),
            shaderByteCode->GetBufferSize(),
            IID_PPV_ARGS(shaderReflection.GetAddressOf())));

        D3D12_SHADER_DESC shaderDesc{};
        ThrowIfFailed(shaderReflection->GetDesc(&shaderDesc));

        if (shaderType == ShaderType::COMPUTE_SHADER) {
            auto SInfo = m_ShaderInfo[name];
            auto CSInfo = std::dynamic_pointer_cast<ComputeShaderInfo>(SInfo);
            shaderReflection->GetThreadGroupSize(
                &CSInfo->m_ThreadGroupSizeX,
                &CSInfo->m_ThreadGroupSizeY,
                &CSInfo->m_ThreadGroupSizeZ);
        }
        

        for (int i = 0; ; ++i) {
            D3D12_SHADER_INPUT_BIND_DESC SIBDesc;
            auto hr = shaderReflection->GetResourceBindingDesc(i, &SIBDesc);
            if (FAILED(hr)) {
                break;
            }

            // 读取常量缓冲区
            if (SIBDesc.Type == D3D_SIT_CBUFFER) {
                GetConstantBufferInfo(shaderReflection.Get(), SIBDesc, shaderType, name);
            }
            else if (SIBDesc.Type == D3D_SIT_TEXTURE ||
                SIBDesc.Type == D3D_SIT_TBUFFER ||
                SIBDesc.Type == D3D_SIT_BYTEADDRESS ||
                SIBDesc.Type == D3D_SIT_STRUCTURED) {
                GetShaderResourceInfo(SIBDesc, shaderType, name);
            }
            else if (SIBDesc.Type == D3D_SIT_UAV_RWTYPED ||
                SIBDesc.Type == D3D_SIT_UAV_RWSTRUCTURED ||
                SIBDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER ||
                SIBDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED ||
                SIBDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED ||
                SIBDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS) {
                GetRWResourceInfo(SIBDesc, shaderType, name);
            }
            else if (SIBDesc.Type == D3D_SIT_SAMPLER) {
                GetSamplerStateInfo(SIBDesc, shaderType, name);
            }
        }
    }

    void ShaderHealper::Impl::GetConstantBufferInfo(
        ID3D12ShaderReflection* shaderReflection,
        const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
        ShaderType shaderType,
        const std::string& name)
    {
        ID3D12ShaderReflectionConstantBuffer* CBReflection = shaderReflection->GetConstantBufferByName(bindDesc.Name);
        D3D12_SHADER_BUFFER_DESC CBDesc;
        ThrowIfFailed(CBReflection->GetDesc(&CBDesc));
        
        auto infoName = std::to_string(static_cast<int>(shaderType)) + name;
        auto& shaderInfo = m_ShaderInfo;
    
        ConstantBuffer constantBuffer{};
        constantBuffer.m_Data.resize(CBDesc.Size);
        constantBuffer.m_Name = CBDesc.Name;
        constantBuffer.m_BindPoint = bindDesc.BindPoint;
        constantBuffer.m_RegisterSpace = bindDesc.Space;
        bool noParams = strcmp(bindDesc.Name, "$Params") != 0;
        
        // 判断该常量缓冲区是否是参数常量缓冲区
        if (noParams) {
            // 不是参数CB
            if (auto it = m_ConstantBuffers.find(bindDesc.BindPoint); it == m_ConstantBuffers.end()) {
                // 不存在则新建
                m_ConstantBuffers.emplace(std::make_pair(bindDesc.BindPoint, std::move(constantBuffer)));
            }
            else if (it->second.m_Data.size() < CBDesc.Size) {
                // 存在但是大小比反射的小，可能是宏导致的
                m_ConstantBuffers[bindDesc.BindPoint] = std::move(constantBuffer);
            }
            
            // 标记该着色器使用了当前常量缓冲区
            shaderInfo[infoName]->m_CbUseMask |= (1 << bindDesc.BindPoint);
        }
        else if (CBDesc.Variables > 0){
            // 若是参数CB为其创建一个常量缓冲区
            shaderInfo[infoName]->m_pParamData = std::make_unique<ConstantBuffer>(std::move(constantBuffer));
        }

        // 获取常量缓冲区中的所有变量
        for (std::uint32_t i = 0; i< CBDesc.Variables; i++) {
            ID3D12ShaderReflectionVariable* pSRVar = CBReflection->GetVariableByIndex(i);
            D3D12_SHADER_VARIABLE_DESC varDesc;
            ThrowIfFailed(pSRVar->GetDesc(&varDesc));

            auto CBVariable = std::make_shared<ConstantBufferVariable>();
            CBVariable->m_Name = varDesc.Name;
            CBVariable->m_ByteSize = varDesc.Size;
            CBVariable->m_StartOffset = varDesc.StartOffset;
            if (noParams) {
                CBVariable->m_ConstantBuffer = &m_ConstantBuffers[bindDesc.BindPoint];
                m_ConstantBufferVariables[varDesc.Name] = CBVariable;
                if (varDesc.DefaultValue) { // 设置初始值
                    m_ConstantBufferVariables[varDesc.Name]->SetRow(varDesc.Size, varDesc.DefaultValue);
                }
            }
            else {
                // 若是着色器参数则独属于该着色器                
                CBVariable->m_ConstantBuffer = shaderInfo[infoName]->m_pParamData.get();
                shaderInfo[infoName]->m_ConstantBufferVariable[varDesc.Name] = CBVariable;
            }
        }
    }

    void ShaderHealper::Impl::GetShaderResourceInfo(
        const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
        ShaderType shaderType,
        const std::string& name)
    {
        auto infoName = std::to_string(static_cast<int>(shaderType)) + name;

        if (auto it = m_ShaderResources.find(bindDesc.BindPoint); it == m_ShaderResources.end()) {
            ShaderResource shaderResource{};
            shaderResource.m_Name = bindDesc.Name;
            shaderResource.m_BindPoint = bindDesc.BindPoint;
            shaderResource.m_RegisterSpace = bindDesc.Space;
            shaderResource.m_Dimension = static_cast<D3D12_SRV_DIMENSION>( bindDesc.Dimension);
            m_ShaderResources[bindDesc.BindPoint] = std::move(shaderResource);
        }

        m_ShaderInfo[infoName]->m_SrUseMasks[bindDesc.BindPoint / 32] |= (1 << (bindDesc.BindPoint % 32));
    }

    void ShaderHealper::Impl::GetRWResourceInfo(
        const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
        ShaderType shaderType,
        const std::string& name)
    {
        auto infoName = std::to_string(static_cast<int>(shaderType)) + name;

        if (auto it = m_RWResources.find(bindDesc.BindPoint); it == m_RWResources.end()) {
            RWResource rwResource{};
            rwResource.m_Name = bindDesc.Name;
            rwResource.m_BindPoint = bindDesc.BindPoint;
            rwResource.m_RegisterSpace = bindDesc.Space;
            rwResource.m_Dimension = static_cast<D3D12_UAV_DIMENSION>(bindDesc.Dimension);
            rwResource.m_FirstInit = false;
            rwResource.m_EnableCounter = bindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER;
            rwResource.m_InitialCount = 0;
            m_RWResources[bindDesc.BindPoint] = std::move(rwResource);
        }

        if (shaderType == ShaderType::PIXEL_SHADER) {
            auto shaderInfo = std::dynamic_pointer_cast<PixelShaderInfo>(m_ShaderInfo[infoName]);
            shaderInfo->m_RwUseMask |= (1 << bindDesc.BindPoint);
        }
        else if (shaderType == ShaderType::COMPUTE_SHADER) {
            auto shaderInfo = std::dynamic_pointer_cast<ComputeShaderInfo>(m_ShaderInfo[infoName]);
            shaderInfo->m_RwUseMask |= (1 << bindDesc.BindPoint);
        }
    }

    void ShaderHealper::Impl::GetSamplerStateInfo(
        const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
        ShaderType shaderType,
        const std::string& name)
    {
        auto infoName = std::to_string(static_cast<int>(shaderType)) + name;

        if (auto it = m_ShaderResources.find(bindDesc.BindPoint); it == m_ShaderResources.end()) {
            SamplerState samplerState{};
            samplerState.m_Name = bindDesc.Name;
            samplerState.m_BindPoint = bindDesc.BindPoint;
            samplerState.m_RegisterSpace = bindDesc.Space;
            m_SamplerStates[bindDesc.BindPoint] = std::move(samplerState);
        }

        m_ShaderInfo[infoName]->m_SsUseMask |= (1 << bindDesc.BindPoint);
    }



    

    ShaderHealper::ShaderHealper()
        :m_Impl(std::make_unique<Impl>()){
    }

    ShaderHealper::~ShaderHealper()
    {
    }

    void ShaderHealper::SetFrameCount(std::uint32_t frameCount)
    {
        FrameCount = frameCount;
    }

    void ShaderHealper::CreateShaderFormFile(const ShaderDesc& shaderDesc)
    {
        ComPtr<ID3DBlob> byteCode = nullptr;
        ComPtr<ID3DBlob> errors = nullptr;
        UINT compilFlags = D3DCOMPILE_ENABLE_STRICTNESS;	// 强制严格编译

#if defined(DEBUG) || defined(_DEBUG) 
        compilFlags |= D3DCOMPILE_DEBUG;
        compilFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;	// 跳过优化步骤
#endif

        auto hr = D3DCompileFromFile(
            AnsiToWString(shaderDesc.m_FileName).c_str(),
            shaderDesc.m_Defines.GetShaderDefines().data(),
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            shaderDesc.m_EnterPoint.c_str(),
            shaderDesc.m_Target.c_str(),
            compilFlags,
            0,
            byteCode.GetAddressOf(),
            errors.GetAddressOf());

        if (errors != nullptr) {
            OutputDebugStringA(static_cast<char*>(errors->GetBufferPointer()));
        }
        ThrowIfFailed(hr);

        if (!shaderDesc.m_OutputFileName.empty()) {
            ThrowIfFailed(D3DWriteBlobToFile(byteCode.Get(), AnsiToWString(shaderDesc.m_OutputFileName).c_str(), TRUE));
        }

        m_Impl->m_ShaderPassByteCode[shaderDesc.m_FileName] = byteCode;
        auto shaderInfoName= std::to_string(static_cast<int>(shaderDesc.m_Type)) + shaderDesc.m_ShaderName;

        std::shared_ptr<ShaderInfo> shaderInfo;
        switch (shaderDesc.m_Type) {
            case ShaderType::PIXEL_SHADER: {
                shaderInfo = std::make_shared<PixelShaderInfo>();break;
            }
            case ShaderType::COMPUTE_SHADER: {
                shaderInfo = std::make_shared<ComputeShaderInfo>();break;
            }
            default: {
                shaderInfo = std::make_shared<ShaderInfo>();break;
            }
        }
        m_Impl->m_ShaderInfo[shaderInfoName] = shaderInfo;
        m_Impl->m_ShaderInfo[shaderInfoName]->m_Name = shaderDesc.m_ShaderName;
        m_Impl->m_ShaderInfo[shaderInfoName]->m_pShader = byteCode;

        m_Impl->GetShaderInfo(shaderDesc.m_ShaderName, shaderDesc.m_Type, byteCode.Get());
    }
    
}
