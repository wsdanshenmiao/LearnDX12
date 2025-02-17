#pragma once
#ifndef __OBJECTMANAGER__H__
#define __OBJECTMANAGER__H__

#include "Object.h"
#include "Singleton.h"
#include "D3DUtil.h"
#include "ModelManager.h"

class CpuTimer;

namespace DSM {
	class ObjectManager : public Singleton<ObjectManager>
	{
	public:
		bool AddObject(std::shared_ptr<Object> object) noexcept;
		bool RemoveObject(std::shared_ptr<Object> object) noexcept;

		std::size_t GetObjectCount() const noexcept;

		// 创建所有对象的常量缓冲区资源
		template<typename ObjCB>
		void CreateObjectsResource(ID3D12Device* device);

		template<typename CBFunc>
		void UpdateObjectsCB(const CpuTimer& timer, CBFunc func);
		
		template<typename VertexData>
		void RenderObjects(ID3D12GraphicsCommandList* cmdList, UINT CBIndex);
		
	protected:
		friend class Singleton<ObjectManager>;
		ObjectManager(UINT frameCount);
		virtual ~ObjectManager() = default;

	protected:
		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		
		ComPtr<ID3D12Resource> m_ObjCB;
		
		const UINT m_FrameCount;
		UINT m_Counter;
		UINT m_ObjCBByteSize;
		
		std::map<std::string, std::shared_ptr<Object>> m_Objects;
	};


	template<typename ObjCB>
	inline void ObjectManager::CreateObjectsResource(ID3D12Device* device)
	{
		// 创建常量缓冲区资源
		m_ObjCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjCB));
		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Alignment = 0;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Width = m_ObjCBByteSize * m_Objects.size() * m_FrameCount;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.SampleDesc ={1,0};
		resourceDesc.MipLevels = 1;

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_ObjCB.ReleaseAndGetAddressOf())));
	}

	template<typename CBFunc>
	inline void ObjectManager::UpdateObjectsCB(const CpuTimer& timer, CBFunc func)
	{
		m_Counter = (m_Counter + 1) % m_FrameCount;
		
		auto frameByteSize = m_ObjCBByteSize * m_Objects.size();
		BYTE* mappedData = nullptr;
		ThrowIfFailed(m_ObjCB->Map(0, nullptr,reinterpret_cast<void**>(&mappedData)))
		int objIndex = 0;
		for (const auto& [name, object] : m_Objects) {
			decltype(auto) cb = func(*object);
			memcpy(mappedData + (m_Counter * frameByteSize + objIndex * m_ObjCBByteSize),&cb, m_ObjCBByteSize);
			objIndex++;
		}
		m_ObjCB->Unmap(0,nullptr);
	}

	template<typename VertexData>
	inline void ObjectManager::RenderObjects(ID3D12GraphicsCommandList* cmdList, UINT CBIndex)
	{
		int counter = 0;
		auto frameByteSize = m_ObjCBByteSize * m_Objects.size();
		for (const auto& [name, obj] : m_Objects) {
			if (auto model = obj->GetModel(); model != nullptr) {
				const auto& meshData = ModelManager::GetInstance().GetMeshData<VertexData>(model->GetName());
				auto vertexBV = meshData->GetVertexBufferView();
				auto indexBV = meshData->GetIndexBufferView();
				cmdList->IASetVertexBuffers(0, 1, &vertexBV);
				cmdList->IASetIndexBuffer(&indexBV);

				auto handle = m_ObjCB->GetGPUVirtualAddress();
				handle += m_Counter * frameByteSize + counter * m_ObjCBByteSize;
				cmdList->SetGraphicsRootConstantBufferView(CBIndex, handle);
				
				for (const auto& [name, drawItem] : meshData->m_DrawArgs) {
					cmdList->DrawIndexedInstanced(drawItem.m_IndexCount, 1, drawItem.m_StarIndexLocation, drawItem.m_BaseVertexLocation, 0);
				}
			}
			counter++;
		}
	}


	

}

#endif // !__OBJECTMANAGER__H__
