#pragma once
#ifndef __TEXTUREMANAGER__H__
#define __TEXTUREMANAGER__H__

#include "Singleton.h"
#include "Texture.h"
#include "D3D12Allocatioin.h"
#include "FrameResource.h"

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

		size_t GetTextureCount() const noexcept;
		const std::unordered_map<std::string, Texture>& GetAllTextures() noexcept;
		D3D12DescriptorHandle GetTextureResourceView(const std::string& texName) const;
		D3D12DescriptorHandle GetDefaultTextureResourceView() const;
		ID3D12DescriptorHeap* GetDescriptorHeap() const;

	protected:
		friend class Singleton<TextureManager>;
		TextureManager(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
		virtual ~TextureManager() = default;

		void CreateSRV(Texture& texture);

	protected:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		
		std::unique_ptr<D3D12UploadBufferAllocator> m_UploadBufferAllocator;
		std::unique_ptr<D3D12TextureAllocator> m_TextureAllocator;
		std::unique_ptr<D3D12DescriptorHeap> m_DescriptorHeap;
		
		std::unordered_map<std::string, Texture> m_Textures;
	};
}

#endif