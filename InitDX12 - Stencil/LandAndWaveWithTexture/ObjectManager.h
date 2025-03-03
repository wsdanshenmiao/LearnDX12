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
		ID3D12Resource* CreateObjectsResource(
			const std::string& name,
			ID3D12Device* device,
			UINT CBByteSize,
			UINT matCBByteSize) const;

		template<typename ObjCBFunc, typename MatCBFunc>
		void UpdateObjectsCB(std::string name, ObjCBFunc objFunc, MatCBFunc matFunc, ID3D12Resource* resource);
		
	protected:
		friend class Singleton<ObjectManager>;
		ObjectManager() = default;
		virtual ~ObjectManager() = default;

	protected:
		std::map<std::string, std::shared_ptr<Object>> m_Objects;
	};

	template<typename ObjCBFunc, typename MatCBFunc>
	inline void ObjectManager::UpdateObjectsCB(std::string name, ObjCBFunc objFunc, MatCBFunc matFunc, ID3D12Resource* resource)
	{
		if (resource == nullptr || !m_Objects.contains(name))return;

		const auto& object = m_Objects[name];
		if (object->GetModel() == nullptr) return;
		
		BYTE* mappedData = nullptr;
		ThrowIfFailed(resource->Map(0, nullptr,reinterpret_cast<void**>(&mappedData)))
		decltype(auto) objCB = objFunc(*object);
		auto objByteSize = D3DUtil::CalcCBByteSize(sizeof(objCB));
		memcpy(mappedData, &objCB, objByteSize);
		std::size_t counter = 0;
		for (const auto& mat : object->GetModel()->GetAllMaterial()) {
			decltype(auto) matCB = matFunc(mat);
			auto matByteSize = D3DUtil::CalcCBByteSize(sizeof(matCB));
			memcpy(mappedData + objByteSize + counter++ * matByteSize, &matCB, matByteSize);
		}
		resource->Unmap(0,nullptr);
	}

}

#endif // !__OBJECTMANAGER__H__
