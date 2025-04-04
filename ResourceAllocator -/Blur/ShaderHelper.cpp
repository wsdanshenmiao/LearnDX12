#include "ShaderHelper.h"
#include "FrameResource.h"
#include "D3DUtil.h"


using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace DSM {

	struct ShaderParameter
	{
		std::string m_Name;
		ShaderParameterIndex m_ParamIndex;
	};


	// 着色器资源
	struct ShaderResource : ShaderParameter
	{
		std::vector<D3D12DescriptorHandle> m_Handle;
		std::uint32_t m_BindCount;
		D3D12_SRV_DIMENSION m_Dimension;
	};

	// 可读写资源
	struct RWResource : ShaderParameter
	{
		std::vector<D3D12DescriptorHandle> m_Handle;
		std::uint32_t m_BindCount;
		D3D12_UAV_DIMENSION m_Dimension;
		uint32_t m_InitialCount;
		bool m_EnableCounter;
		bool m_FirstInit;
	};

	struct SamplerState : ShaderParameter
	{
		std::vector<D3D12DescriptorHandle> m_Handle;
	};

	// 常量缓冲区
	struct ConstantBuffer : ShaderParameter
	{
		bool m_IsDrty;
		std::vector<std::uint8_t> m_Data;
		std::shared_ptr<D3D12ResourceLocation> m_Resource;

		// 更新常量缓冲区
		void UpdateBuffer()
		{
			if (m_IsDrty) {
				assert(m_Resource != nullptr);
				memcpy(m_Resource->m_MappedBaseAddress, m_Data.data(), m_Data.size());
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

		void SetFloat2(const DirectX::XMFLOAT2& val) override
		{
			SetRow(sizeof(DirectX::XMFLOAT2), &val);
		}

		void SetFloat3(const DirectX::XMFLOAT3& val) override
		{
			SetRow(sizeof(DirectX::XMFLOAT3), &val);
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

			m_ConstantBuffer->m_IsDrty = true;
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
		std::unique_ptr<ConstantBuffer> m_pParamData = nullptr; // 每个着色器有自己的参数常量缓冲区
		std::map<std::string, std::shared_ptr<ConstantBufferVariable>> m_ConstantBufferVariable;    // 常量缓冲区变量 
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
		m_DefinesDxc[AnsiToWString(name)] = AnsiToWString(value);
	}

	void ShaderDefines::RemoveDefine(const std::string& name)
	{
		if (m_Defines.contains(name)) {
			m_Defines.erase(name);
			m_DefinesDxc.erase(AnsiToWString(name));
		}
	}

	std::vector<D3D_SHADER_MACRO> ShaderDefines::GetShaderDefines() const
	{
		std::vector<D3D_SHADER_MACRO> defines;
		defines.reserve(m_Defines.size() + 1);
		for (const auto& define : m_Defines) {
			defines.push_back({ define.first.c_str(), define.second.c_str() });
		}
		defines.push_back({ nullptr, nullptr });
		return defines;
	}

	std::vector<DxcDefine> ShaderDefines::GetShaderDefinesDxc() const
	{
		std::vector<DxcDefine> defines;
		defines.reserve(m_DefinesDxc.size());
		for (const auto& define : m_DefinesDxc) {
			defines.push_back({ define.first.c_str(), define.second.c_str() });
		}
		return defines;
	}


#pragma region Shader Pass
	//
	// ShaderPass Implementation
	//
	struct ShaderPass : IShaderPass
	{
		enum ParamType : int
		{
			CONSTANTBUFFER = 0, TEXTURE = 1, SAMPLER = 2, UAV = 3, NUMTYPES
		};


		ShaderPass(ShaderHelper* shaderHelper,
			const std::string& passName,
			std::map<std::uint32_t, ConstantBuffer>& cBuffers,
			std::map<std::uint32_t, ShaderResource>& shaderResources,
			std::map<std::uint32_t, RWResource>& rwResources,
			std::map<std::uint32_t, SamplerState>& samplerStates);

		void SetBlendState(const D3D12_BLEND_DESC& blendDesc) override;
		void SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc) override;
		void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc) override;
		void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout) override;
		void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) override;
		void SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc) override;
		void SetSampleDesc(std::uint32_t count, std::uint32_t quality) override;
		void SetDSVFormat(DXGI_FORMAT format) override;
		void SetRTVFormat(const std::vector<DXGI_FORMAT>& formats) override;
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GetPSODesc() const  override;

		CBVariableSP VSGetParamByName(const std::string& paramName) override;
		CBVariableSP DSGetParamByName(const std::string& paramName) override;
		CBVariableSP HSGetParamByName(const std::string& paramName) override;
		CBVariableSP GSGetParamByName(const std::string& paramName) override;
		CBVariableSP PSGetParamByName(const std::string& paramName) override;
		CBVariableSP CSGetParamByName(const std::string& paramName) override;
		CBVariableSP GetParamByName(const std::string& paramName, ShaderType type);

		const ShaderHelper* GetShaderHelper() const override;
		const std::string& GetPassName() const override;

		void Dispatch(ID3D12GraphicsCommandList* cmdList, std::uint32_t threadX, std::uint32_t threadY,
			std::uint32_t threadZ) override;
		void Apply(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device, FrameResource* frameResourc) override;



		std::string m_PassName{};
		const ShaderHelper* m_pShaderHealper = nullptr;

		// 着色器信息
		std::vector<std::shared_ptr<ShaderInfo>> m_ShaderInfos{};

		// 来自Shader中的共用资源
		std::map<std::uint32_t, ConstantBuffer>& m_CBuffers;
		std::map<std::uint32_t, ShaderResource>& m_ShaderResources;
		std::map<std::uint32_t, RWResource>& m_RWResources;
		std::map<std::uint32_t, SamplerState>& m_SamplerStates;

		ComPtr<ID3D12RootSignature> m_pRootSignature = nullptr;	

		// 用于最后生成PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_GraphicsPSODesc{};
		D3D12_COMPUTE_PIPELINE_STATE_DESC m_ComputePSODesc{};
		// 一趟Pass有一个PSO， 可能会有重复的PSO，暂不考虑
		ComPtr<ID3D12PipelineState> m_pGraphicsPSO = nullptr;
		ComPtr<ID3D12PipelineState> m_pComputePSO = nullptr;
		std::array<std::vector<std::uint32_t>, NUMTYPES> m_RootParamIndexs{};


	private:
		void CreateRootSignature(ID3D12Device* device);
		const std::array<const D3D12_STATIC_SAMPLER_DESC, 7> CreateStaticSamplers() const noexcept;

	};

	ShaderPass::ShaderPass(ShaderHelper* shaderHelper,
		const std::string& passName,
		std::map<std::uint32_t, ConstantBuffer>& cBuffers,
		std::map<std::uint32_t, ShaderResource>& shaderResources,
		std::map<std::uint32_t, RWResource>& rwResources,
		std::map<std::uint32_t, SamplerState>& samplerStates)
		:m_PassName(passName),
		m_pShaderHealper(shaderHelper),
		m_CBuffers(cBuffers),
		m_ShaderResources(shaderResources),
		m_RWResources(rwResources),
		m_SamplerStates(samplerStates) {
		m_ShaderInfos.resize(static_cast<int>(ShaderType::NUM_SHADER_TYPES));

		// 设置默认的PSO
		ZeroMemory(&m_GraphicsPSODesc, sizeof(m_GraphicsPSODesc));
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = false;
		blendDesc.RenderTarget[0].LogicOpEnable = false;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ForcedSampleCount = 0;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
			D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		depthStencilDesc.FrontFace = defaultStencilOp;
		depthStencilDesc.BackFace = defaultStencilOp;

		m_GraphicsPSODesc.SampleMask = UINT_MAX;
		m_GraphicsPSODesc.BlendState = blendDesc;
		m_GraphicsPSODesc.RasterizerState = rasterizerDesc;
		m_GraphicsPSODesc.DepthStencilState = depthStencilDesc;
		m_GraphicsPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_GraphicsPSODesc.SampleDesc = { 1, 0 };
		m_GraphicsPSODesc.NumRenderTargets = 1;
		m_GraphicsPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_GraphicsPSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		ZeroMemory(&m_ComputePSODesc, sizeof(m_ComputePSODesc));
	}

	void ShaderPass::SetBlendState(const D3D12_BLEND_DESC& blendDesc)
	{
		m_GraphicsPSODesc.BlendState = blendDesc;
	}

	void ShaderPass::SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
	{
		m_GraphicsPSODesc.RasterizerState = rasterizerDesc;
	}

	void ShaderPass::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
	{
		m_GraphicsPSODesc.DepthStencilState = depthStencilDesc;
	}

	void ShaderPass::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout)
	{
		m_GraphicsPSODesc.InputLayout = inputLayout;
	}

	void ShaderPass::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
	{
		m_GraphicsPSODesc.PrimitiveTopologyType = topology;
	}

	void ShaderPass::SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc)
	{
		m_GraphicsPSODesc.SampleDesc = sampleDesc;
	}

	void ShaderPass::SetSampleDesc(std::uint32_t count, std::uint32_t quality)
	{
		m_GraphicsPSODesc.SampleDesc = { count, quality };
	}

	void ShaderPass::SetDSVFormat(DXGI_FORMAT format)
	{
		m_GraphicsPSODesc.DSVFormat = format;
	}

	void ShaderPass::SetRTVFormat(const std::vector<DXGI_FORMAT>& formats)
	{
		assert(formats.size() <= 8);

		for (std::size_t i = 0; i < formats.size(); ++i) {
			m_GraphicsPSODesc.RTVFormats[i] = formats[i];
		}
		m_GraphicsPSODesc.NumRenderTargets = formats.size();
	}

	const D3D12_GRAPHICS_PIPELINE_STATE_DESC& ShaderPass::GetPSODesc() const
	{
		return m_GraphicsPSODesc;
	}

	ShaderPass::CBVariableSP ShaderPass::VSGetParamByName(const std::string& paramName)
	{
		return GetParamByName(paramName, ShaderType::VERTEX_SHADER);
	}

	ShaderPass::CBVariableSP ShaderPass::DSGetParamByName(const std::string& paramName)
	{
		return GetParamByName(paramName, ShaderType::DOMAIN_SHADER);
	}

	ShaderPass::CBVariableSP ShaderPass::HSGetParamByName(const std::string& paramName)
	{
		return GetParamByName(paramName, ShaderType::HULL_SHADER);
	}

	ShaderPass::CBVariableSP ShaderPass::GSGetParamByName(const std::string& paramName)
	{
		return GetParamByName(paramName, ShaderType::GEOMETRY_SHADER);
	}

	ShaderPass::CBVariableSP ShaderPass::PSGetParamByName(const std::string& paramName)
	{
		return GetParamByName(paramName, ShaderType::PIXEL_SHADER);
	}

	ShaderPass::CBVariableSP ShaderPass::CSGetParamByName(const std::string& paramName)
	{
		return GetParamByName(paramName, ShaderType::COMPUTE_SHADER);
	}

	ShaderPass::CBVariableSP ShaderPass::GetParamByName(const std::string& paramName, ShaderType type)
	{
		auto info = m_ShaderInfos[static_cast<int>(type)];
		if (info != nullptr) {
			if (auto it = info->m_ConstantBufferVariable.find(paramName); it != info->m_ConstantBufferVariable.end()) {
				return it->second;
			}
		}
		return nullptr;
	}

	const ShaderHelper* ShaderPass::GetShaderHelper() const
	{
		return m_pShaderHealper;
	}

	const std::string& ShaderPass::GetPassName() const
	{
		return m_PassName;
	}

	void ShaderPass::Dispatch(
		ID3D12GraphicsCommandList* cmdList,
		std::uint32_t threadX,
		std::uint32_t threadY,
		std::uint32_t threadZ)
	{
		assert(cmdList != nullptr);

		auto pSInfo = m_ShaderInfos[static_cast<int>(ShaderType::COMPUTE_SHADER)];
		if (pSInfo == nullptr) {
#ifdef _DEBUG
			OutputDebugStringA("[Warning]: No compute shader in current effect pass!");
#endif
			return;
		}

		auto pCSInfo = std::dynamic_pointer_cast<ComputeShaderInfo>(pSInfo);
		// 将线程组数量进行对齐
		std::uint32_t threadGroupCountX = (threadX + pCSInfo->m_ThreadGroupSizeX - 1) / pCSInfo->m_ThreadGroupSizeX;
		std::uint32_t threadGroupCountY = (threadY + pCSInfo->m_ThreadGroupSizeY - 1) / pCSInfo->m_ThreadGroupSizeY;
		std::uint32_t threadGroupCountZ = (threadZ + pCSInfo->m_ThreadGroupSizeZ - 1) / pCSInfo->m_ThreadGroupSizeZ;

		cmdList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void ShaderPass::Apply(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device, FrameResource* frameResource)
	{
		assert(cmdList != nullptr);

		auto getIndex = [](const auto& type) {
			return static_cast<int>(type);
			};

		// 未生成根签名或PSO则生成
		if (m_pRootSignature == nullptr || m_pGraphicsPSO == nullptr) {
			CreateRootSignature(device);

			// 创建渲染管线对象
			m_GraphicsPSODesc.pRootSignature = m_pRootSignature.Get();
			if (auto VS = m_ShaderInfos[getIndex(ShaderType::VERTEX_SHADER)]; VS != nullptr) {
				m_GraphicsPSODesc.VS = { VS->m_pShader->GetBufferPointer(), VS->m_pShader->GetBufferSize() };
			}
			if (auto HS = m_ShaderInfos[getIndex(ShaderType::HULL_SHADER)]; HS != nullptr) {
				m_GraphicsPSODesc.HS = { HS->m_pShader->GetBufferPointer(), HS->m_pShader->GetBufferSize() };
			}
			if (auto DS = m_ShaderInfos[getIndex(ShaderType::DOMAIN_SHADER)]; DS != nullptr) {
				m_GraphicsPSODesc.DS = { DS->m_pShader->GetBufferPointer(), DS->m_pShader->GetBufferSize() };
			}
			if (auto GS = m_ShaderInfos[getIndex(ShaderType::GEOMETRY_SHADER)]; GS != nullptr) {
				m_GraphicsPSODesc.GS = { GS->m_pShader->GetBufferPointer(), GS->m_pShader->GetBufferSize() };
			}
			if (auto PS = m_ShaderInfos[getIndex(ShaderType::PIXEL_SHADER)]; PS != nullptr) {
				m_GraphicsPSODesc.PS = { PS->m_pShader->GetBufferPointer(), PS->m_pShader->GetBufferSize() };
			}

			ThrowIfFailed(device->CreateGraphicsPipelineState(&m_GraphicsPSODesc, IID_PPV_ARGS(m_pGraphicsPSO.GetAddressOf())));
		}

		if (m_ShaderInfos[getIndex(ShaderType::COMPUTE_SHADER)] != nullptr && m_pComputePSO == nullptr) {
			m_ComputePSODesc.pRootSignature - m_pRootSignature.Get();
			auto CS = m_ShaderInfos[getIndex(ShaderType::COMPUTE_SHADER)]->m_pShader;
			m_ComputePSODesc.CS = { CS->GetBufferPointer(), CS->GetBufferSize() };

			ThrowIfFailed(device->CreateComputePipelineState(&m_ComputePSODesc, IID_PPV_ARGS(m_pComputePSO.GetAddressOf())));
		}


		// 设置资源
		cmdList->SetPipelineState(m_pGraphicsPSO.Get());
		cmdList->SetGraphicsRootSignature(m_pRootSignature.Get());

		std::uint32_t index = 0;
		// 绑定常量缓冲区
		for (auto& cbIndex : m_RootParamIndexs[ParamType::CONSTANTBUFFER]) {
			auto& cb = m_CBuffers[cbIndex];
			if (cb.m_Resource != nullptr) {
				cb.UpdateBuffer();
				cmdList->SetGraphicsRootConstantBufferView(index++, cb.m_Resource->m_GPUVirtualAddress);
			}
		}
		// 绑定着色器参数中的常量缓冲区
		for (auto& shaderInfo : m_ShaderInfos) {
			if (shaderInfo != nullptr && shaderInfo->m_pParamData != nullptr) {
				// 更新参数常量缓冲区
				shaderInfo->m_pParamData->UpdateBuffer();
				cmdList->SetGraphicsRootConstantBufferView(
					shaderInfo->m_pParamData->m_ParamIndex.m_BindPoint, shaderInfo->m_pParamData->m_Resource->m_GPUVirtualAddress);
			}
		}

		// 绑定着色器资源
		auto& descriptorCache = frameResource->m_DescriptorCaches;
		ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorCache->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);

		for (auto& srIndex : m_RootParamIndexs[ParamType::TEXTURE]) {
			auto& sr = m_ShaderResources[srIndex];
			auto handleSize = sr.m_Handle.size();
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuHandles(handleSize);
			for (int i = 0; i < handleSize; ++i) {
				assert(sr.m_Handle[i].IsValid());
				cpuHandles[i] = sr.m_Handle[i];
			}
			auto gpuHandle = descriptorCache->AllocateAndCopy(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandles);
			cmdList->SetGraphicsRootDescriptorTable(index++, gpuHandle);
		}
		for (auto& rwindex : m_RootParamIndexs[ParamType::UAV]) {
			auto& rw = m_RWResources[rwindex];
			auto handleSize = rw.m_Handle.size();
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuHandles(handleSize);
			for (int i = 0; i < handleSize; ++i) {
				assert(rw.m_Handle[i].IsValid());
				cpuHandles[i] = rw.m_Handle[i];
			}
			auto gpuHandle = descriptorCache->AllocateAndCopy(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandles);
			cmdList->SetGraphicsRootDescriptorTable(index++, gpuHandle);
		}

		//for (int i = 0; i < m_SamplerStates.size(); ++i) {
		//	auto handleSize = m_SamplerStates[i].m_Handle.size();
		//	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> cpuHandles(handleSize);
		//	for (int j = 0; j < handleSize; ++j) {
		//		assert(m_ShaderResources[i].m_Handle[j].IsValid());
		//		cpuHandles[j] = m_SamplerStates[i].m_Handle[j];
		//	}
		//	auto gpuHandle = descriptorCache->AllocateAndCopy(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, cpuHandles);
		//	cmdList->SetGraphicsRootDescriptorTable(m_SamplerBindSlot + i, gpuHandle);
		//}

	}


	void ShaderPass::CreateRootSignature(ID3D12Device* device)
	{
		std::vector<D3D12_ROOT_PARAMETER> params{};

		std::uint32_t index = 0;
		auto paramSize = m_CBuffers.size() + m_ShaderResources.size() + m_RWResources.size();
		params.resize(paramSize);
		for (int i = 0; i < ParamType::NUMTYPES; ++i) {
			m_RootParamIndexs[i].clear();
		}

		for (const auto& [paramIndex, cb] : m_CBuffers) {
			D3D12_ROOT_PARAMETER rootParameter{};
			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameter.Constants.ShaderRegister = cb.m_ParamIndex.m_BindPoint;
			rootParameter.Constants.RegisterSpace = cb.m_ParamIndex.m_RegisterSpace;
			params[index++] = std::move(rootParameter);
			m_RootParamIndexs[ParamType::CONSTANTBUFFER].emplace_back(paramIndex);
		}
		
		// 由于根参数储存的是描述符范围的指针，因此需要额外储存下来，否则会使其变成悬空指针
		std::vector<D3D12_DESCRIPTOR_RANGE> srvDescriptorRanges{};
		srvDescriptorRanges.reserve(m_ShaderResources.size());
		for (const auto& [paramIndex, sr] : m_ShaderResources) {
			D3D12_ROOT_PARAMETER rootParameter{};
			D3D12_DESCRIPTOR_RANGE texTable{};
			texTable.NumDescriptors = sr.m_BindCount;
			texTable.BaseShaderRegister = sr.m_ParamIndex.m_BindPoint;
			texTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			texTable.RegisterSpace = sr.m_ParamIndex.m_RegisterSpace;
			texTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			auto& range = srvDescriptorRanges.emplace_back(std::move(texTable));

			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameter.DescriptorTable.NumDescriptorRanges = 1;
			rootParameter.DescriptorTable.pDescriptorRanges = &range;
			params[index++] = std::move(rootParameter);
			m_RootParamIndexs[TEXTURE].emplace_back(paramIndex);
		}

		std::vector<D3D12_DESCRIPTOR_RANGE> uavDescriptorRanges{};
		uavDescriptorRanges.reserve(m_RWResources.size());
		for (const auto& [paramIndex, rw] : m_RWResources) {
			D3D12_ROOT_PARAMETER rootParameter{};
			D3D12_DESCRIPTOR_RANGE texTable{};
			texTable.NumDescriptors = rw.m_BindCount;
			texTable.BaseShaderRegister = rw.m_ParamIndex.m_BindPoint;
			texTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			texTable.RegisterSpace = rw.m_ParamIndex.m_RegisterSpace;
			texTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			auto& range = uavDescriptorRanges.emplace_back(std::move(texTable));

			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameter.DescriptorTable.NumDescriptorRanges = 1;
			rootParameter.DescriptorTable.pDescriptorRanges = &range;
			params[index++] = std::move(rootParameter);
			m_RootParamIndexs[TEXTURE].emplace_back(paramIndex);
		}

		auto staticSamplers = CreateStaticSamplers();
		D3D12_ROOT_SIGNATURE_DESC signatureDesc{};
		signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		signatureDesc.NumParameters = params.size();
		signatureDesc.pParameters = params.data();
		signatureDesc.NumStaticSamplers = (UINT)staticSamplers.size();
		signatureDesc.pStaticSamplers = staticSamplers.data();

		ComPtr<ID3DBlob> errorBlob;
		ComPtr<ID3DBlob> serializedBlob;

		// 序列化根签名
		auto hr = D3D12SerializeRootSignature(&signatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			serializedBlob.GetAddressOf(),
			errorBlob.GetAddressOf());
		if (errorBlob != nullptr) {
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		
		ThrowIfFailed(hr);

		// 创建根签名
		ThrowIfFailed(device->CreateRootSignature(0,
			serializedBlob->GetBufferPointer(),
			serializedBlob->GetBufferSize(),
			IID_PPV_ARGS(m_pRootSignature.GetAddressOf())));
	}

	const std::array<const D3D12_STATIC_SAMPLER_DESC, 7> ShaderPass::CreateStaticSamplers() const noexcept
	{
		// 创建六种静态采样器
		D3D12_STATIC_SAMPLER_DESC staticSampler{};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.MipLODBias = 0;
		staticSampler.MaxAnisotropy = 16;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticSampler.MinLOD = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		staticSampler.ShaderRegister = 0;

		const auto pointWarp = staticSampler;

		staticSampler.ShaderRegister = 1;
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		const auto linearWarp = staticSampler;

		staticSampler.ShaderRegister = 2;
		staticSampler.Filter = D3D12_FILTER_ANISOTROPIC;
		const auto anisotropicWarp = staticSampler;

		staticSampler.ShaderRegister = 5;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		const auto anisotropicClamp = staticSampler;

		staticSampler.ShaderRegister = 4;
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		const auto linearClamp = staticSampler;

		staticSampler.ShaderRegister = 3;
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		const auto pointClamp = staticSampler;

		D3D12_STATIC_SAMPLER_DESC shadow{};
		shadow.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		shadow.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		shadow.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		shadow.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		shadow.MipLODBias = 0;
		shadow.MaxAnisotropy = 16;
		shadow.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		shadow.MinLOD = 0;
		shadow.MaxLOD = D3D12_FLOAT32_MAX;
		shadow.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		shadow.ShaderRegister = 6;
		shadow.RegisterSpace = 0;
		shadow.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		return { pointWarp, linearWarp, anisotropicWarp, pointClamp, linearClamp, anisotropicClamp, shadow };
	}
#pragma endregion


#pragma region ShaderHelper
	//
	// ShaderHealper Implementation
	//    
#pragma region ShaderHelper Impl
	struct ShaderHelper::Impl
	{
		~Impl() = default;

		void GetShaderInfo(std::string name, ShaderType shaderType, ID3D12ShaderReflection* shaderReflection);
		void Clear();

		// 存储编译后的 Shader代码
		std::map<std::string, ComPtr<ID3DBlob>> m_ShaderPassByteCode;
		// 着色器的信息
		std::map<std::string, std::shared_ptr<ShaderInfo>> m_ShaderInfo;

		std::map<std::string, std::shared_ptr<IShaderPass>> m_ShaderPass;

		// 各种着色器资源，需要所有着色器的常量缓冲区没有冲突
		std::unordered_map<std::string, std::shared_ptr<ConstantBufferVariable>> m_ConstantBufferVariables;
		std::map<std::uint32_t, ConstantBuffer> m_ConstantBuffers;
		std::map<std::uint32_t, ShaderResource> m_ShaderResources;
		std::map<std::uint32_t, RWResource> m_RWResources;
		std::map<std::uint32_t, SamplerState> m_SamplerStates;

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

	void ShaderHelper::Impl::GetShaderInfo(std::string name, ShaderType shaderType, ID3D12ShaderReflection* shaderReflection)
	{
		ComPtr<ID3D12ShaderReflection> pReflection = shaderReflection;

		D3D12_SHADER_DESC shaderDesc{};
		ThrowIfFailed(pReflection->GetDesc(&shaderDesc));

		if (shaderType == ShaderType::COMPUTE_SHADER) {
			auto SInfo = m_ShaderInfo[name];
			auto CSInfo = std::dynamic_pointer_cast<ComputeShaderInfo>(SInfo);
			pReflection->GetThreadGroupSize(
				&CSInfo->m_ThreadGroupSizeX,
				&CSInfo->m_ThreadGroupSizeY,
				&CSInfo->m_ThreadGroupSizeZ);
		}


		for (int i = 0; ; ++i) {
			D3D12_SHADER_INPUT_BIND_DESC SIBDesc;
			auto hr = pReflection->GetResourceBindingDesc(i, &SIBDesc);
			if (FAILED(hr)) {
				break;
			}

			// 读取常量缓冲区
			if (SIBDesc.Type == D3D_SIT_CBUFFER) {
				GetConstantBufferInfo(pReflection.Get(), SIBDesc, shaderType, name);
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

	void ShaderHelper::Impl::Clear()
	{
		m_ConstantBuffers.clear();
		m_ConstantBufferVariables.clear();
		m_ShaderResources.clear();
		m_ShaderInfo.clear();
		m_ShaderPassByteCode.clear();
		m_SamplerStates.clear();
		m_RWResources.clear();
	}

	void ShaderHelper::Impl::GetConstantBufferInfo(
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
		constantBuffer.m_ParamIndex.m_BindPoint = bindDesc.BindPoint;
		constantBuffer.m_ParamIndex.m_RegisterSpace = bindDesc.Space;
		bool noParams = strcmp(bindDesc.Name, "$Params") != 0;

		auto cbKey = ShaderParameterIndex::GetKeyByIndex(constantBuffer.m_ParamIndex);

		// 判断该常量缓冲区是否是参数常量缓冲区
		if (noParams) {
			// 不是参数CB,则在PassHelper中创建
			if (auto it = m_ConstantBuffers.find(cbKey);
				it == m_ConstantBuffers.end() || it->second.m_Data.size() < CBDesc.Size) {
				// 不存在则新建
				m_ConstantBuffers[cbKey] = std::move(constantBuffer);
			}
		}
		else if (CBDesc.Variables > 0) {
			// 若是参数CB为其创建一个常量缓冲区
			shaderInfo[infoName]->m_pParamData = std::make_unique<ConstantBuffer>(std::move(constantBuffer));
		}

		if (CBDesc.Variables < 1)return;

		ID3D12ShaderReflectionVariable* pSRVar = CBReflection->GetVariableByIndex(0);

		D3D12_SHADER_TYPE_DESC typeDesc;
		ID3D12ShaderReflectionType* reflectionType = pSRVar->GetType();
		ThrowIfFailed(reflectionType->GetDesc(&typeDesc));
		// 检测缓冲区的类型是否为结构体
		bool isStruct = typeDesc.Class == D3D_SVC_STRUCT;

		// 获取常量缓冲区中的所有变量
		std::uint32_t preOffset = 0;
		std::uint32_t numVar = isStruct ? typeDesc.Members : CBDesc.Variables;
		// 用来记录各个变量的大小
		std::queue<std::pair<std::string, std::uint32_t>> sizeQueue;
		for (std::uint32_t i = 0; i < numVar; i++) {
			// 获取变量信息
			D3D12_SHADER_VARIABLE_DESC varDesc;

			std::uint32_t varSize = 0;
			std::uint32_t varOffset = 0;
			std::string varName{};

			// 处理两种语法导致反射的结果不同
			if (isStruct) {
				ID3D12ShaderReflectionType* memberType = reflectionType->GetMemberTypeByIndex(i);
				auto memberName = reflectionType->GetMemberTypeName(i);
				D3D12_SHADER_TYPE_DESC memberTypeDesc;
				ThrowIfFailed(memberType->GetDesc(&memberTypeDesc));

				varName = memberName;
				varSize = 0;
				varOffset = memberTypeDesc.Offset;
				// 当前变量的名字和前一个变量的大小
				sizeQueue.push({ varName, varOffset - preOffset });
				preOffset = varOffset;
			}
			else {
				pSRVar = CBReflection->GetVariableByIndex(i);
				ThrowIfFailed(pSRVar->GetDesc(&varDesc));
				varName = varDesc.Name;
				varSize = varDesc.Size;
				varOffset = varDesc.StartOffset;
			}

			// 创建常量缓冲区成员变量
			auto CBVariable = std::make_shared<ConstantBufferVariable>();
			CBVariable->m_Name = varName;
			CBVariable->m_ByteSize = varSize;
			CBVariable->m_StartOffset = varOffset;
			if (noParams) {
				CBVariable->m_ConstantBuffer = &m_ConstantBuffers[cbKey];
				m_ConstantBufferVariables[CBVariable->m_Name] = CBVariable;
				if (!isStruct && varDesc.DefaultValue != nullptr) { // 设置初始值
					m_ConstantBufferVariables[CBVariable->m_Name]->SetRow(varDesc.Size, varDesc.DefaultValue);
				}
			}
			else {
				// 若是着色器参数则独属于该着色器                
				CBVariable->m_ConstantBuffer = shaderInfo[infoName]->m_pParamData.get();
				shaderInfo[infoName]->m_ConstantBufferVariable[CBVariable->m_Name] = CBVariable;
			}
		}
		if (isStruct) {
			// 设置各个变量的大小
			auto prePair = sizeQueue.front();
			std::string lastName = sizeQueue.back().first;
			sizeQueue.pop();
			for (; !sizeQueue.empty(); sizeQueue.pop()) {
				auto varPair = sizeQueue.front();
				m_ConstantBufferVariables[prePair.first]->m_ByteSize = varPair.second;
				prePair = varPair;
			}
			// 设置最后一个变量的大小
			m_ConstantBufferVariables[lastName]->m_ByteSize = CBDesc.Size - preOffset;
		}
	}

	void ShaderHelper::Impl::GetShaderResourceInfo(
		const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
		ShaderType shaderType,
		const std::string& name)
	{
		auto infoName = std::to_string(static_cast<int>(shaderType)) + name;

		ShaderParameterIndex cbIndex{ bindDesc.BindPoint, bindDesc.Space };
		auto cbKey = ShaderParameterIndex::GetKeyByIndex({ bindDesc.BindPoint, bindDesc.Space });

		if (auto it = m_ShaderResources.find(cbKey); it == m_ShaderResources.end()) {
			ShaderResource shaderResource{};
			shaderResource.m_Name = bindDesc.Name;
			shaderResource.m_ParamIndex = cbIndex;
			shaderResource.m_Dimension = static_cast<D3D12_SRV_DIMENSION>(bindDesc.Dimension);
			shaderResource.m_BindCount = bindDesc.BindCount;
			m_ShaderResources[cbKey] = std::move(shaderResource);
		}
	}

	void ShaderHelper::Impl::GetRWResourceInfo(
		const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
		ShaderType shaderType,
		const std::string& name)
	{
		auto infoName = std::to_string(static_cast<int>(shaderType)) + name;

		ShaderParameterIndex cbIndex{ bindDesc.BindPoint, bindDesc.Space };
		auto cbKey = ShaderParameterIndex::GetKeyByIndex(cbIndex);

		if (auto it = m_RWResources.find(cbKey); it == m_RWResources.end()) {
			RWResource rwResource{};
			rwResource.m_Name = bindDesc.Name;
			rwResource.m_ParamIndex = cbIndex;
			rwResource.m_Dimension = static_cast<D3D12_UAV_DIMENSION>(bindDesc.Dimension);
			rwResource.m_FirstInit = false;
			rwResource.m_EnableCounter = bindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER;
			rwResource.m_InitialCount = 0;
			rwResource.m_BindCount = bindDesc.BindCount;
			m_RWResources[cbKey] = std::move(rwResource);
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

	void ShaderHelper::Impl::GetSamplerStateInfo(
		const D3D12_SHADER_INPUT_BIND_DESC& bindDesc,
		ShaderType shaderType,
		const std::string& name)
	{
		auto infoName = std::to_string(static_cast<int>(shaderType)) + name;

		ShaderParameterIndex cbIndex{ bindDesc.BindPoint, bindDesc.Space };
		auto cbKey = ShaderParameterIndex::GetKeyByIndex(cbIndex);

		if (auto it = m_ShaderResources.find(cbKey); it == m_ShaderResources.end()) {
			SamplerState samplerState{};
			samplerState.m_Name = bindDesc.Name;
			samplerState.m_ParamIndex = cbIndex;
			m_SamplerStates[cbKey] = std::move(samplerState);
		}
	}
#pragma endregion 



	ShaderHelper::ShaderHelper()
		:m_Impl(std::make_unique<Impl>()) {
	}

	ShaderHelper::~ShaderHelper()
	{
	}

	void ShaderHelper::SetConstantBufferByName(const std::string& name, std::shared_ptr<D3D12ResourceLocation> cb)
	{
		auto findByName = [&name](const auto& parame) {
			return parame.second.m_Name == name;
			};
		auto it = std::find_if(m_Impl->m_ConstantBuffers.begin(), m_Impl->m_ConstantBuffers.end(), findByName);
		if (it != m_Impl->m_ConstantBuffers.end()) {
			it->second.m_IsDrty = true;
			it->second.m_Resource = cb;
		}
	}

	void ShaderHelper::SetConstantBufferBySlot(const ShaderParameterIndex& index, std::shared_ptr<D3D12ResourceLocation> cb)
	{
		if (auto it = m_Impl->m_ConstantBuffers.find(ShaderParameterIndex::GetKeyByIndex(index)); 
			it != m_Impl->m_ConstantBuffers.end()) {
			it->second.m_IsDrty = true;
			it->second.m_Resource = cb;
		}
	}

	void ShaderHelper::SetShaderResourceByName(const std::string& name, const std::vector<D3D12DescriptorHandle>& resource)
	{
		auto findByName = [&name](const auto& parame) {
			return parame.second.m_Name == name;
			};
		auto it = std::find_if(m_Impl->m_ShaderResources.begin(), m_Impl->m_ShaderResources.end(), findByName);
		if (it != m_Impl->m_ShaderResources.end()) {
			it->second.m_Handle = resource;
		}
	}

	void ShaderHelper::SetShaderResourceBySlot(const ShaderParameterIndex& index, const std::vector<D3D12DescriptorHandle>& resource)
	{
		if (auto it = m_Impl->m_ShaderResources.find(ShaderParameterIndex::GetKeyByIndex(index)); 
			it != m_Impl->m_ShaderResources.end()) {
			it->second.m_Handle = resource;
		}
	}

	void ShaderHelper::SetRWResourceByName(const std::string& name, const std::vector<D3D12DescriptorHandle>& resource)
	{
		auto findByName = [&name](const auto& parame) {
			return parame.second.m_Name == name;
			};
		auto it = std::find_if(m_Impl->m_RWResources.begin(), m_Impl->m_RWResources.end(), findByName);
		if (it != m_Impl->m_RWResources.end()) {
			it->second.m_Handle = resource;
		}
	}

	void ShaderHelper::SetRWResrouceBySlot(const ShaderParameterIndex& index, const std::vector<D3D12DescriptorHandle>& resource)
	{
		if (auto it = m_Impl->m_RWResources.find(ShaderParameterIndex::GetKeyByIndex(index)); 
			it != m_Impl->m_RWResources.end()) {
			it->second.m_Handle = resource;
		}
	}

	void ShaderHelper::SetSampleStateByName(const std::string& name, const std::vector<D3D12DescriptorHandle>& sampleState)
	{
		auto findByName = [&name](const auto& parame) {
			return parame.second.m_Name == name;
			};
		auto it = std::find_if(m_Impl->m_SamplerStates.begin(), m_Impl->m_SamplerStates.end(), findByName);
		if (it != m_Impl->m_SamplerStates.end()) {
			it->second.m_Handle = sampleState;
		}
	}

	void ShaderHelper::SetSampleStateBySlot(const ShaderParameterIndex& index, const std::vector<D3D12DescriptorHandle>& sampleState)
	{
		if (auto it = m_Impl->m_SamplerStates.find(ShaderParameterIndex::GetKeyByIndex(index)); 
			it != m_Impl->m_SamplerStates.end()) {
			it->second.m_Handle = sampleState;
		}
	}

	ShaderPass::CBVariableSP ShaderHelper::GetConstantBufferVariable(const std::string& name)
	{
		auto it = m_Impl->m_ConstantBufferVariables.find(name);
		return it == m_Impl->m_ConstantBufferVariables.end() ? nullptr : it->second;
	}

	void ShaderHelper::AddShaderPass(
		const std::string& shaderPassName,
		const ShaderPassDesc& passDesc,
		ID3D12Device* device)
	{
		assert(!shaderPassName.empty());

		auto it = m_Impl->m_ShaderPass.find(shaderPassName);
		// 防止重复添加
		assert(it == m_Impl->m_ShaderPass.end());

		auto shaderPass = std::make_shared<ShaderPass>(
			this, shaderPassName,
			m_Impl->m_ConstantBuffers, m_Impl->m_ShaderResources,
			m_Impl->m_RWResources, m_Impl->m_SamplerStates);
		m_Impl->m_ShaderPass[shaderPassName] = shaderPass;

		// 加入着色器信息
		auto getIndex = [](const auto& type) {
			return static_cast<int>(type);
			};
		auto index = getIndex(ShaderType::VERTEX_SHADER);
		shaderPass->m_ShaderInfos[index] = m_Impl->m_ShaderInfo[std::to_string(index) + passDesc.m_VSName];
		index = getIndex(ShaderType::HULL_SHADER);
		shaderPass->m_ShaderInfos[index] = m_Impl->m_ShaderInfo[std::to_string(index) + passDesc.m_HSName];
		index = getIndex(ShaderType::DOMAIN_SHADER);
		shaderPass->m_ShaderInfos[index] = m_Impl->m_ShaderInfo[std::to_string(index) + passDesc.m_DSName];
		index = getIndex(ShaderType::GEOMETRY_SHADER);
		shaderPass->m_ShaderInfos[index] = m_Impl->m_ShaderInfo[std::to_string(index) + passDesc.m_GSName];
		index = getIndex(ShaderType::PIXEL_SHADER);
		shaderPass->m_ShaderInfos[index] = m_Impl->m_ShaderInfo[std::to_string(index) + passDesc.m_PSName];
		index = getIndex(ShaderType::COMPUTE_SHADER);
		shaderPass->m_ShaderInfos[index] = m_Impl->m_ShaderInfo[std::to_string(index) + passDesc.m_CSName];
	}

	std::shared_ptr<IShaderPass> ShaderHelper::GetShaderPass(const std::string& passName)
	{
		if (auto it = m_Impl->m_ShaderPass.find(passName); it != m_Impl->m_ShaderPass.end()) {
			return it->second;
		}
		return nullptr;
	}

	void ShaderHelper::CreateShaderFormFile(const ShaderDesc& shaderDesc)
	{
		auto shaderInfoName = std::to_string(static_cast<int>(shaderDesc.m_Type)) + shaderDesc.m_ShaderName;

		std::shared_ptr<ShaderInfo> shaderInfo;
		switch (shaderDesc.m_Type) {
		case ShaderType::PIXEL_SHADER: {
			shaderInfo = std::make_shared<PixelShaderInfo>(); break;
		}
		case ShaderType::COMPUTE_SHADER: {
			shaderInfo = std::make_shared<ComputeShaderInfo>(); break;
		}
		default: {
			shaderInfo = std::make_shared<ShaderInfo>(); break;
		}
		}
		m_Impl->m_ShaderInfo[shaderInfoName] = shaderInfo;
		m_Impl->m_ShaderInfo[shaderInfoName]->m_Name = shaderDesc.m_ShaderName;

		ComPtr<ID3D12ShaderReflection> reflection;
		ComPtr<ID3DBlob> byteCode = nullptr;
		if (byteCode = DXCCreateShaderFromFile(shaderDesc, reflection.GetAddressOf()); byteCode == nullptr) {
			byteCode = D3DCompileCreateShaderFromFile(shaderDesc, reflection.GetAddressOf());
		}

		m_Impl->GetShaderInfo(shaderDesc.m_ShaderName, shaderDesc.m_Type, reflection.Get());

		m_Impl->m_ShaderInfo[shaderInfoName]->m_pShader = byteCode;

		m_Impl->m_ShaderPassByteCode[shaderDesc.m_FileName] = byteCode;
	}

	void ShaderHelper::Clear()
	{
		m_Impl->Clear();
	}

	ComPtr<ID3DBlob> ShaderHelper::DXCCreateShaderFromFile(const ShaderDesc& shaderDesc, ID3D12ShaderReflection** reflection)
	{
		ComPtr<IDxcUtils> pUtils;
		ComPtr<IDxcCompiler3> pCompiler;
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

		ComPtr<IDxcIncludeHandler> pIncludeHandler;
		ThrowIfFailed(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler));

		auto fileName = AnsiToWString(shaderDesc.m_FileName);
		auto entryPoint = AnsiToWString(shaderDesc.m_EnterPoint);
		auto pTarget = AnsiToWString(shaderDesc.m_Target);

		auto defines = shaderDesc.m_Defines.GetShaderDefinesDxc();
		ComPtr<IDxcCompilerArgs> pArgs;
		ThrowIfFailed(pUtils->BuildArguments(
			fileName.c_str(),
			entryPoint.c_str(),
			pTarget.c_str(),
			nullptr, 0,
			defines.data(),
			defines.size(),
			pArgs.GetAddressOf()));


		ComPtr<IDxcBlobEncoding> pSource = nullptr;
		ThrowIfFailed(pUtils->LoadFile(fileName.c_str(), nullptr, &pSource));
		DxcBuffer Source;
		Source.Ptr = pSource->GetBufferPointer();
		Source.Size = pSource->GetBufferSize();
		Source.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

		//
		// Compile it with specified arguments.
		//
		ComPtr<IDxcResult> pResults;
		ThrowIfFailed(pCompiler->Compile(
			&Source,                // Source buffer.
			pArgs->GetArguments(),                // Array of pointers to arguments.
			pArgs->GetCount(),      // Number of arguments.
			pIncludeHandler.Get(),  // User-provided interface to handle #include directives (optional).
			IID_PPV_ARGS(pResults.GetAddressOf()) // Compiler output status, buffer, and errors.
		));

		//
		// Print errors if present.
		//
		ComPtr<IDxcBlobUtf8> pErrors = nullptr;
		ThrowIfFailed(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
		// Note that d3dcompiler would return null if no errors or warnings are present.  
		// IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
		if (pErrors != nullptr && pErrors->GetStringLength() != 0) {
			auto str = pErrors->GetStringPointer();
			wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());
			return nullptr;
		}

		//
		// Quit if the compilation failed.
		//
		HRESULT hrStatus;
		ThrowIfFailed(pResults->GetStatus(&hrStatus));
		if (FAILED(hrStatus)) {
			wprintf(L"Compilation Failed\n");
			return nullptr;
		}

		//
		// Save shader binary.
		//
		ComPtr<IDxcBlob> pShader = nullptr;
		ComPtr<IDxcBlobUtf16> pShaderName = nullptr;
		ThrowIfFailed(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName));

		if (reflection != nullptr) {
			ComPtr<IDxcContainerReflection> pContainerReflection = nullptr;
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&pContainerReflection)));
			ThrowIfFailed(pContainerReflection->Load(pShader.Get()));
			std::uint32_t reflectIndex;
			ThrowIfFailed(pContainerReflection->FindFirstPartKind(DXC_PART_DXIL, &reflectIndex));
			ThrowIfFailed(pContainerReflection->GetPartReflection(reflectIndex, IID_PPV_ARGS(reflection)));
		}

		ComPtr<ID3DBlob> pByteCode = nullptr;
		ThrowIfFailed(pShader->QueryInterface(IID_PPV_ARGS(pByteCode.GetAddressOf())));

		return pByteCode.Get();
	}

	ComPtr<ID3DBlob> ShaderHelper::D3DCompileCreateShaderFromFile(const ShaderDesc& shaderDesc, ID3D12ShaderReflection** reflection)
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

		// 获取着色器反射
		if (reflection != nullptr) {
			ThrowIfFailed(D3DReflect(byteCode->GetBufferPointer(),
				byteCode->GetBufferSize(),
				IID_PPV_ARGS(reflection)));
		}

		return byteCode;
	}

#pragma endregion 

}
