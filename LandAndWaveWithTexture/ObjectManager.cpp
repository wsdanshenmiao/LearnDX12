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

	bool ObjectManager::RemoveObject(std::shared_ptr<Object> object) noexcept
	{
		if (object == nullptr || !m_Objects.contains(object->GetName())) {
			return false;
		}
		else {
			m_Objects.erase(object->GetName());
			return true;
		}
	}

	std::size_t ObjectManager::GetObjectCount() const noexcept
	{
		return m_Objects.size();
	}

	ObjectManager::ObjectManager(UINT frameCount)
		: m_FrameCount(frameCount), m_Counter(0), m_ObjCBByteSize(0) {
	}




}
