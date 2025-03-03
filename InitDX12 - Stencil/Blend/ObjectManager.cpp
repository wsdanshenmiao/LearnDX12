#include "ObjectManager.h"
#include "CpuTimer.h"
#include "ModelManager.h"

namespace DSM {
	bool ObjectManager::AddObject(std::shared_ptr<Object> object, RenderLayer layer) noexcept
	{
		if (layer >= RenderLayer::Count) {
			return false;
		}
		if (object == nullptr || m_Objects[(int)layer].contains(object->GetName())) {
			return false;
		}
		else {
			m_Objects[(int)layer][object->GetName()] = object;
			return true;
		}
	}

	std::vector<ObjectManager::ObjectMap>& ObjectManager::GetAllObject() noexcept
	{
		return m_Objects;
	}

	std::size_t ObjectManager::GetObjectCount() const noexcept
	{
		return m_Objects.size();
	}

	std::size_t ObjectManager::GetMaterialCount() const noexcept
	{
		std::size_t count = 0;
		for (const auto& objs : m_Objects) {
			for (const auto&[name, object] : objs) {
				if (auto model = object->GetModel();model != nullptr) {
					count += model->GetMaterialSize();
				}
			}
		}
		return count;
	}

	std::size_t ObjectManager::GetObjectWithModelCount() const noexcept
	{
		std::size_t count = 0;
		for (const auto& objs : m_Objects) {
			for (const auto&[name, object] : objs) {
				if (auto model = object->GetModel();model != nullptr) {
					++count;
				}
			}
		}
		return count;
	}

	void ObjectManager::CreateObjectsResource(FrameResource* frameResource, UINT objCBByteSize, UINT matCBByteSize) const
	{
		if (objCBByteSize == 0 || matCBByteSize == 0) return;

		objCBByteSize = D3DUtil::CalcCBByteSize(objCBByteSize);
		matCBByteSize = D3DUtil::CalcCBByteSize(matCBByteSize);

		for (const auto& objs : m_Objects) {
			for (const auto& [name, object] : objs) {
				auto model = object->GetModel();
				if (model == nullptr) continue;

				// 创建常量缓冲区资源
				auto byteSize = objCBByteSize + model->GetAllMaterial().size() * matCBByteSize;

				frameResource->AddConstantBuffer(byteSize, 1, name);
			}
		}
	}
	
	ObjectManager::ObjectManager()
	{
		m_Objects.resize((int)RenderLayer::Count);
	}



}
