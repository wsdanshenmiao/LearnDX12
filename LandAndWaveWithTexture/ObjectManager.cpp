#include "ObjectManager.h"
#include "CpuTimer.h"
#include "ModelManager.h"

namespace DSM {
	bool ObjectManager::AddObject(std::shared_ptr<Object> object) noexcept
	{
		if (object == nullptr || m_Objects.contains(object->GetName())) {
			return false;
		}
		else {
			m_Objects[object->GetName()] = object;
			return true;
		}
	}

	std::map<std::string, std::shared_ptr<Object>>& ObjectManager::GetAllObject() noexcept
	{
		return m_Objects;
	}

	/*bool ObjectManager::RemoveObject(std::shared_ptr<Object> object) noexcept
	{
		if (object == nullptr || !m_Objects.contains(object->GetName())) {
			return false;
		}
		else {
			m_Objects.erase(object->GetName());
			return true;
		}
	}*/

	std::size_t ObjectManager::GetObjectCount() const noexcept
	{
		return m_Objects.size();
	}

	std::size_t ObjectManager::GetMaterialCount() const noexcept
	{
		std::size_t count = 0;
		for (const auto&[name, object] : m_Objects) {
			if (auto model = object->GetModel();model != nullptr) {
				count += model->GetMaterialSize();
			}
		}
		return count;
	}

	std::size_t ObjectManager::GetObjectWithModelCount() const noexcept
	{
		std::size_t count = 0;
		for (const auto&[name, object] : m_Objects) {
			if (auto model = object->GetModel();model != nullptr) {
				++count;
			}
		}
		return count;
	}

	ID3D12Resource* ObjectManager::CreateObjectsResource(
		const std::string& name,
		ID3D12Device* device,
		UINT objCBByteSize,
		UINT matCBByteSize) const
	{
		ID3D12Resource* resource = nullptr;

		if (device == nullptr ||
			objCBByteSize == 0 ||
			matCBByteSize == 0 ||
			!m_Objects.contains(name)) return resource;

		const auto& obj = m_Objects.find(name)->second;
		if (obj->GetModel() == nullptr) return resource;

		objCBByteSize = D3DUtil::CalcCBByteSize(objCBByteSize);
		matCBByteSize = D3DUtil::CalcCBByteSize(matCBByteSize);
		
		// 创建常量缓冲区资源
		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Alignment = 0;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Width = objCBByteSize + obj->GetModel()->GetAllMaterial().size() * matCBByteSize;
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
			IID_PPV_ARGS(&resource)));

		return 	resource;
	}
	




}
