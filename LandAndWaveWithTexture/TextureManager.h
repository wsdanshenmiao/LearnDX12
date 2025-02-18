#pragma once
#ifndef __TEXTUREMANAGER__H__
#define __TEXTUREMANAGER__H__

#include "Singleton.h"
#include "Texture.h"

namespace DSM {
	class TextureManager : public Singleton<TextureManager>
	{
	public:
		bool LoadTextureFromFile(
			const std::string& fileName,
			ID3D12GraphicsCommandList* cmdList);
		bool LoadTextureFromMemory(
			const std::string& name,
			void* data,
			size_t dataSize,
			ID3D12GraphicsCommandList* cmdList);
		bool AddTexture(const std::string& name, Texture&& texture);

		size_t GetTextureSize() const noexcept;
		std::unordered_map<std::string, Texture>& GetAllTextures() noexcept;

	protected:
		friend class Singleton<TextureManager>;
		TextureManager(ID3D12Device* device);
		virtual ~TextureManager() = default;

	protected:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		std::unordered_map<std::string, Texture> m_Textures;
	};
}

#endif