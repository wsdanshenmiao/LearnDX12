#pragma once
#ifndef __TEXTURE__H__
#define __TEXTURE__H__

#include "D3D12Allocatioin.h"
#include "D3D12Resource.h"
#include "Pubh.h"

namespace DSM {
	class Texture
	{
	public:
		Texture() noexcept = default;
		Texture(const std::string& name) noexcept;
		Texture(const std::string& name,
			const std::string& fileName,   
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12TextureAllocator* texAllocator,
			D3D12UploadBufferAllocator* uploadAllocator);
		
		const std::string& GetName() const noexcept;
		UINT GetDescriptorIndex() const noexcept;
		D3D12ResourceLocation&  GetTexture();
		const D3D12ResourceLocation& GetTexture() const;
		
		void SetName(const std::string& name);
		void SetTexture(const D3D12ResourceLocation& texture);
		void SetUploadHeap(const D3D12ResourceLocation& uploadHeap);
		void SetDescriptorIndex(UINT index) noexcept;
		
		void DisposeUploader() noexcept;
		
		static bool LoadTextureFromFile(
			Texture& texture,
			const std::string& fileName,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12TextureAllocator* texAllocator,
			D3D12UploadBufferAllocator* uploadAllocator);
		static bool LoadTextureFromFile(
			Texture& texture,
			const std::string& name,
			const std::string& fileName,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12TextureAllocator* texAllocator,
			D3D12UploadBufferAllocator* uploadAllocator);
		static bool LoadTextureFromMemory(
			Texture& texture,
			const std::string& name,
			void* data,
			size_t dataSize,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12TextureAllocator* texAllocator,
			D3D12UploadBufferAllocator* uploadAllocator);

	private:
		static void LoadTexture(
			Texture& texture,
			const std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12UploadBufferAllocator* uploadAllocator);
		
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		D3D12ResourceLocation m_Texture{};
		D3D12ResourceLocation m_UploadHeap{};
		
		std::string m_Name;
		UINT m_DescriptorIndex = -1;
	};

	
}

#endif