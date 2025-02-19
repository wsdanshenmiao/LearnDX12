#include "TextureManager.h"


namespace DSM {
	bool TextureManager::LoadTextureFromFile(
		const std::string& fileName,
		ID3D12GraphicsCommandList* cmdList)
	{
		return LoadTextureFromFile(fileName, fileName, cmdList);
	}

	bool TextureManager::LoadTextureFromFile(
		const std::string& name,
		const std::string& fileName,
		ID3D12GraphicsCommandList* cmdList)
	{
		if (m_Textures.contains(fileName)) return false;

		Texture tex{};
		bool sucess = Texture::LoadTextureFromFile(tex, name, fileName, m_Device.Get(), cmdList);
		if (sucess) {
			tex.SetDescriptorIndex(m_Textures.size());
			m_Textures[name] = std::move(tex);
		}
		return sucess;
	}

	bool TextureManager::LoadTextureFromMemory(
		const std::string& name,
		void* data, size_t dataSize,
		ID3D12GraphicsCommandList* cmdList)
	{
		if (m_Textures.contains(name)) return false;

		Texture tex{};
		bool sucess = Texture::LoadTextureFromMemory(tex, name, data, dataSize, m_Device.Get(), cmdList);
		if (sucess) {
			tex.SetDescriptorIndex(m_Textures.size());
			m_Textures[name] = std::move(tex);
		}
		return sucess;
	}

	bool TextureManager::AddTexture(const std::string& name, Texture&& texture)
	{
		if (m_Textures.contains(name)) {
			return false;
		}
		else {
			texture.SetDescriptorIndex(m_Textures.size());
			m_Textures[name] = std::move(texture);
			return true;
		}
	}

	size_t TextureManager::GetTextureSize() const noexcept
	{
		return m_Textures.size();
	}

	const std::unordered_map<std::string, Texture>& TextureManager::GetAllTextures() noexcept
	{
		return m_Textures;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureResourceView(
		const std::string& texName,
		UINT descriptorSize) const
	{
		auto handle = m_TexDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		// 若是不包含该纹理则返回第一个纹理
		if (m_Textures.contains(texName)) {
			handle.ptr += m_Textures.find(texName)->second.GetDescriptorIndex() * descriptorSize;
		}
		return handle;
	}

	void TextureManager::CreateTexDescriptor(UINT descriptorSize)
	{
		// 创建描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NumDescriptors = m_Textures.size();
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_TexDescriptorHeap.ReleaseAndGetAddressOf()));

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0;
		for (auto& [name, tex] : m_Textures) {
			SRVDesc.Format = tex.GetTexture()->GetDesc().Format;
			SRVDesc.Texture2D.MipLevels = tex.GetTexture()->GetDesc().MipLevels;

			auto handle=m_TexDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			handle.ptr += tex.GetDescriptorIndex() * descriptorSize;
			
			m_Device->CreateShaderResourceView(tex.GetTexture(),&SRVDesc,handle);
		}
	}

	TextureManager::TextureManager(ID3D12Device* device)
		:m_Device(device) {
	}

}
