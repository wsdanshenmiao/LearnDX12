#pragma once
#ifndef __TEXTUREMANAGER__H__
#define __TEXTUREMANAGER__H__

#include "Singleton.h"
#include "Texture.h"

namespace DSM {
	class TextureManager : public Singleton<TextureManager>
	{
	public:
		const Texture* LoadTextureFromFile(
			const std::string& fileName,
			ID3D12GraphicsCommandList* cmdList);
		const Texture* LoadTextureFromFile(
			const std::string& name,
			const std::string& fileName,
			ID3D12GraphicsCommandList* cmdList);
		const Texture* LoadTextureFromMemory(
			const std::string& name,
			void* data,
			size_t dataSize,
			ID3D12GraphicsCommandList* cmdList);
		bool AddTexture(const std::string& name, Texture&& texture);

		size_t GetTextureSize() const noexcept;
		const std::unordered_map<std::string, Texture>& GetAllTextures() noexcept;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureResourceView(
			const std::string& texName,
			UINT descriptorSize) const;
		ID3D12DescriptorHeap* GetDescriptorHeap() const;

		void CreateTexDescriptor(UINT descriptorSize);

	protected:
		friend class Singleton<TextureManager>;
		TextureManager(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
		virtual ~TextureManager() = default;

	protected:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_TexDescriptorHeap;
		std::unordered_map<std::string, Texture> m_Textures;
	};
}

#endif