#pragma once
#ifndef __TEXTURE__H__
#define __TEXTURE__H__

#include "Pubh.h"

namespace DSM {
	struct Texture
	{
		std::string m_Name;
		std::wstring m_Filename;

		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		ComPtr<ID3D12Resource> m_Texture = nullptr;
		ComPtr<ID3D12Resource> m_UploadHeap = nullptr;
	};
}

#endif