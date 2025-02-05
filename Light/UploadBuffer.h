#ifndef __UPLOADBUFFER__H__
#define __UPLOADBUFFER__H__

#include "D3DUtil.h"

namespace DSM {

	template<typename T>
	class UploadBuffer
	{
	public:
		template <class P>
		using ComPtr = Microsoft::WRL::ComPtr<P>;
		UploadBuffer(ID3D12Device* device, UINT elementByteSize, UINT elementCount, bool isConstantBuffer);
		UploadBuffer(const UploadBuffer& other) = delete;
		UploadBuffer(UploadBuffer&& other) = default;
		UploadBuffer& operator=(const UploadBuffer& other) = delete;
		UploadBuffer& operator=(UploadBuffer&& other) = default;

		void Map();
		void Unmap();
		ID3D12Resource* GetResource();
		void CopyData(int elementIndex, const T* const data, const std::size_t& byteSize);

	public:
		bool m_IsDirty = false;

	private:
		ComPtr<ID3D12Resource> m_UploadBuffer;
		BYTE* m_MappedData = nullptr;
		UINT m_ElementByteSize = 0;
		bool m_IsConstantBuffer = false;
	};


	template<typename T>
	inline UploadBuffer<T>::UploadBuffer(
		ID3D12Device* device,
		UINT elementByteSize,
		UINT elementCount,
		bool isConstantBuffer)
		:m_IsConstantBuffer(isConstantBuffer) {
		m_ElementByteSize = m_IsConstantBuffer ?
			D3DUtil::CalcConstantBufferByteSize(elementByteSize) : elementByteSize;
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_UploadBuffer.GetAddressOf())));
	}

	template<typename T>
	inline void UploadBuffer<T>::Map()
	{
		ThrowIfFailed(m_UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
	}

	template<typename T>
	inline void UploadBuffer<T>::Unmap()
	{
		m_UploadBuffer->Unmap(0, nullptr);
	}

	template<typename T>
	inline ID3D12Resource* UploadBuffer<T>::GetResource()
	{
		return m_UploadBuffer.Get();
	}

	template<typename T>
	inline void UploadBuffer<T>::CopyData(int elementIndex, const T* const data, const std::size_t& byteSize)
	{
		if (m_IsDirty) {
			memcpy(&m_MappedData[m_ElementByteSize * elementIndex], data, byteSize);
			m_IsDirty = false;
		}
	}



}

#endif // !__UPLOADBUFFER__H__
