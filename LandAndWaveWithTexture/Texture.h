#pragma once
#ifndef __TEXTURE__H__
#define __TEXTURE__H__

#include "Pubh.h"

namespace DSM {
	class Texture
	{
	public:
		Texture() noexcept = default;
		Texture(const std::string& name) noexcept;
		Texture(const std::string& name,
			const std::wstring& fileName,   
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList);
		
		const std::string& GetName() const noexcept;
		ID3D12Resource* GetTexture();
		const ID3D12Resource* GetTexture() const;
		
		void SetName(const std::string& name);
		
		void DisposeUploader() noexcept;
		
		static bool LoadTextureFromFile(
			Texture& texture,
			const std::wstring& fileName,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList);
		
	private:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		ComPtr<ID3D12Resource> m_Texture = nullptr;
		ComPtr<ID3D12Resource> m_UploadHeap = nullptr;
		
		std::string m_Name;
	};

	
}

#endif