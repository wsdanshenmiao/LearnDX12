#pragma once
#ifndef __SHADERHELPER__H__
#define __SHADERHELPER__H__

#include <wrl/client.h>
#include <dxcapi.h>
#include <DirectXMath.h>
#include <map>

#include "D3D12DescriptorHeap.h"
#include "D3D12Resource.h"

struct ID3D12ShaderReflection;

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
		virtual void SetFloat2(const DirectX::XMFLOAT2& val) = 0;
		virtual void SetFloat3(const DirectX::XMFLOAT3& val) = 0;
		virtual void SetVector(const DirectX::XMFLOAT4& val) = 0;
		virtual void SetMatrix(const DirectX::XMFLOAT4X4& val) = 0;
		virtual void SetRow(std::uint32_t byteSize, const void* data, std::uint32_t offset = 0) = 0;

		virtual ~IConstantBufferVariable() = default;
	};

	class ShaderHelper;
	struct IShaderPass
	{
		using CBVariableSP = std::shared_ptr<IConstantBufferVariable>;
		
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
		virtual CBVariableSP VSGetParamByName(const std::string& paramName) = 0;
		virtual CBVariableSP DSGetParamByName(const std::string& paramName) = 0;
		virtual CBVariableSP HSGetParamByName(const std::string& paramName) = 0;
		virtual CBVariableSP GSGetParamByName(const std::string& paramName) = 0;
		virtual CBVariableSP PSGetParamByName(const std::string& paramName) = 0;
		virtual CBVariableSP CSGetParamByName(const std::string& paramName) = 0;

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
		using HandleArray = std::vector<D3D12DescriptorHandle>;

		
		ShaderHelper();
		~ShaderHelper();
		ShaderHelper(const ShaderHelper&) = delete;
		ShaderHelper& operator=(const ShaderHelper&) = delete;

		void SetConstantBufferByName(const std::string& name, std::shared_ptr<D3D12ResourceLocation> cb);
		void SetConstantBufferBySlot(std::uint32_t slot, std::shared_ptr<D3D12ResourceLocation> cb);
		void SetShaderResourceByName(const std::string& name, const HandleArray& resource);
		void SetShaderResourceBySlot(std::uint32_t slot, const HandleArray& resource);
		void SetRWResourceByName(const std::string& name, const HandleArray& resource);
		void SetRWResrouceBySlot(std::uint32_t slot, const HandleArray& resource);
		void SetSampleStateByName(const std::string& name, const HandleArray& sampleState);
		void SetSampleStateBySlot(std::uint32_t slot, const HandleArray& sampleState);

		std::shared_ptr<IConstantBufferVariable> GetConstantBufferVariable(const std::string& name);

		void AddShaderPass(const std::string& shaderPassName, const ShaderPassDesc& passDesc, ID3D12Device* device);
		std::shared_ptr<IShaderPass> GetShaderPass(const std::string& passName);

		void CreateShaderFormFile(const ShaderDesc& shaderDesc);
		void Clear();

		Microsoft::WRL::ComPtr<ID3DBlob> DXCCreateShaderFromFile(const ShaderDesc& shaderDesc, ID3D12ShaderReflection** reflection);
		Microsoft::WRL::ComPtr<ID3DBlob> D3DCompileCreateShaderFromFile(const ShaderDesc& shaderDesc, ID3D12ShaderReflection** reflection);

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};

	
}

#endif