#include "TextureManager.h"


namespace DSM {
	bool TextureManager::LoadTextureFromFile(
		const std::string& fileName,
		ID3D12GraphicsCommandList* cmdList)
	{
		if (m_Textures.contains(fileName)) return false;

		Texture tex{};
		bool sucess = Texture::LoadTextureFromFile(tex, fileName, m_Device.Get(), cmdList);
		if (sucess) {
			m_Textures[fileName] = std::move(tex);
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
			m_Textures[name] = std::move(texture);
			return true;
		}
	}

	size_t TextureManager::GetTextureSize() const noexcept
	{
		return m_Textures.size();
	}

	std::unordered_map<std::string, Texture>& TextureManager::GetAllTextures() noexcept
	{
		return m_Textures;
	}

	TextureManager::TextureManager(ID3D12Device* device)
		:m_Device(device) {
	}

}
