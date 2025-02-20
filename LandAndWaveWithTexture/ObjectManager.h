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
		//bool RemoveObject(std::shared_ptr<Object> object) noexcept;

		std::map<std::string, std::shared_ptr<Object>>& GetAllObject() noexcept;
		std::size_t GetObjectCount() const noexcept;
		std::size_t GetMaterialCount() const noexcept;
		std::size_t GetObjectWithModelCount() const noexcept;

		// 创建所有对象的常量缓冲区资源
		ID3D12Resource* CreateObjectsResource(ID3D12Device* device, UINT CBSize);

		template<typename CBFunc>
		void UpdateObjectsCB(const CpuTimer& timer, CBFunc func, ID3D12Resource* resource);
		template<typename CBFunc>
		void UpdateMaterialsCB(const CpuTimer& timer, CBFunc func, ID3D12Resource* resource);
		
	protected:
		friend class Singleton<ObjectManager>;
		ObjectManager() = default;
		virtual ~ObjectManager() = default;

	protected:
		std::map<std::string, std::shared_ptr<Object>> m_Objects;
	};

	template<typename CBFunc>
	inline void ObjectManager::UpdateObjectsCB(const CpuTimer& timer, CBFunc func, ID3D12Resource* resource)
	{
		if (resource == nullptr)return;
		
		BYTE* mappedData = nullptr;
		ThrowIfFailed(resource->Map(0, nullptr,reinterpret_cast<void**>(&mappedData)))
		for (const auto& [name, object] : m_Objects) {
			decltype(auto) cb = func(*object);
			auto byteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(cb));
			memcpy(mappedData + object->GetCBIndex() * byteSize, &cb, byteSize);
		}
		resource->Unmap(0,nullptr);
	}

	template <typename CBFunc>
	inline void ObjectManager::UpdateMaterialsCB(const CpuTimer& timer, CBFunc func, ID3D12Resource* resource)
	{
		if (resource == nullptr)return;

		BYTE* mappedData = nullptr;
		UINT objCounter = 0;
		ThrowIfFailed(resource->Map(0, nullptr,reinterpret_cast<void**>(&mappedData)));
		for (const auto& [name, object] : m_Objects) {
			auto model = object->GetModel();
			if (model != nullptr) {
				for (const auto& mat : model->GetAllMaterial()) {
					decltype(auto) cb = func(mat);
					auto byteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(cb));
					memcpy(mappedData + object->GetCBIndex() * byteSize, &cb, byteSize);
				}
			}
		}
		resource->Unmap(0, nullptr);
	}
}

#endif // !__OBJECTMANAGER__H__
