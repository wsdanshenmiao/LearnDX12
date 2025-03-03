#pragma once
#ifndef __OBJECTMANAGER__H__
#define __OBJECTMANAGER__H__

#include "Object.h"
#include "Singleton.h"
#include "D3DUtil.h"
#include "FrameResource.h"
#include "ModelManager.h"

class CpuTimer;

namespace DSM {
	enum class RenderLayer : int
	{
		Opaque = 0,
		Transparent = 1,
		AlphaTest = 2,
		Mirror = 3,
		Reflection = 4,
		Count
	};
	
	class ObjectManager : public Singleton<ObjectManager>
	{
	public:
		using ObjectMap = std::map<std::string, std::shared_ptr<Object>>;
		
		bool AddObject(std::shared_ptr<Object> object, RenderLayer layer) noexcept;
		//bool RemoveObject(std::shared_ptr<Object> object) noexcept;

		std::vector<ObjectMap>& GetAllObject() noexcept;
		const std::shared_ptr<Object> GetObjectByName(const std::string& name) const;
		std::shared_ptr<Object> GetObjectByName(const std::string& name);
		const std::shared_ptr<Object> GetObjectByName(const std::string& name, RenderLayer layer) const;
		std::shared_ptr<Object> GetObjectByName(const std::string& name, RenderLayer layer);
		std::size_t GetObjectCount() const noexcept;
		std::size_t GetMaterialCount() const noexcept;
		std::size_t GetObjectWithModelCount() const noexcept;

		// 创建所有对象的常量缓冲区资源
		void CreateObjectsResource(FrameResource* frameResource, UINT CBByteSize, UINT matCBByteSize) const;

		template<typename ObjCBFunc, typename MatCBFunc>
		void UpdateObjectsCB(
			FrameResource* frameReource,
			ObjCBFunc objFunc,
			MatCBFunc matFunc);
		
	protected:
		friend class Singleton<ObjectManager>;
		ObjectManager();
		virtual ~ObjectManager() = default;

	protected:
		std::vector<ObjectMap> m_Objects;
	};

	template<typename ObjCBFunc, typename MatCBFunc>
	inline void ObjectManager::UpdateObjectsCB(
		FrameResource* frameReource,
		ObjCBFunc objFunc,
		MatCBFunc matFunc)
	{
		if (frameReource == nullptr)return;

		for (std::size_t i = 0; i < m_Objects.size(); ++i) {
			for (auto& [name, obj] : m_Objects[i]) {
				auto it = frameReource->m_Buffers.find(name);
				if (it == frameReource->m_Buffers.end() || obj->GetModel() == nullptr) continue;
				
				auto resource = it->second;
					
				BYTE* mappedData = nullptr;
				ThrowIfFailed(resource->Map(0, nullptr,reinterpret_cast<void**>(&mappedData)))
					
				decltype(auto) objCB = objFunc(*obj, static_cast<RenderLayer>(i));
				auto objByteSize = D3DUtil::CalcCBByteSize(sizeof(objCB));
				memcpy(mappedData, &objCB, objByteSize);
					
				std::size_t counter = 0;
				for (const auto& mat : obj->GetModel()->GetAllMaterial()) {
					decltype(auto) matCB = matFunc(mat);
					auto matByteSize = D3DUtil::CalcCBByteSize(sizeof(matCB));
					memcpy(mappedData + objByteSize + counter++ * matByteSize, &matCB, matByteSize);
				}
					
				resource->Unmap(0,nullptr);
			}
		}
	}

}

#endif // !__OBJECTMANAGER__H__
