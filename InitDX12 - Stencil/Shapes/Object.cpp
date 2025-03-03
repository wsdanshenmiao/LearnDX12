#include "Object.h"
#include "D3DUtil.h"

using namespace DSM::Geometry;

namespace DSM {
	Object::Object(const std::string& name) noexcept
		:m_Name(name) {
	}

	Object::Object(const std::string& name, std::shared_ptr<Geometry::GeometryMesh> geometryMesh) noexcept
		:m_Name(name), m_GeometryMesh(geometryMesh) {
	}

	Object::~Object()
	{
		if (m_Parent) {
			m_Parent->m_ChildObject.erase(m_Name);
			m_Parent = nullptr;
		}
		for (auto& child : m_ChildObject) {
			if (child.second) {
				child.second->m_Parent = nullptr;
				delete child.second;
			}
		}
	}

	Transform& Object::GetTransform() noexcept
	{
		return m_Transform;
	}

	const Transform& Object::GetTransform() const noexcept
	{
		return m_Transform;
	}

	const std::shared_ptr<Geometry::GeometryMesh> Object::GetRenderItem() const noexcept
	{
		return m_GeometryMesh;
	}

	DirectX::BoundingBox Object::GetBouningBox() const noexcept
	{
		return m_BoundingBox;
	}

	void Object::AddChild(Object* child) noexcept
	{
		if (child->m_Parent && child->m_Parent != this) {
			child->m_Parent->m_ChildObject.erase(m_Name);
		}
		child->m_Parent = this;
		m_ChildObject.try_emplace(child->m_Name, child);
	}

	void Object::SetParent(Object* parent) noexcept
	{
		if (m_Parent) {
			m_Parent->m_ChildObject.erase(m_Name);
		}
		m_Parent = parent;
		parent->m_ChildObject[m_Name] = this;
	}

	void Object::SetBoundingBox(const DirectX::BoundingBox& boundingBox) noexcept
	{
		m_BoundingBox = boundingBox;
	}

	void Object::SetGeometryMesh(std::shared_ptr<Geometry::GeometryMesh> pMesh) noexcept
	{
		m_GeometryMesh = pMesh;
	}

}