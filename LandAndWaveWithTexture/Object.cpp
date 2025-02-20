#include "Object.h"
#include "D3DUtil.h"

using namespace DirectX;
using namespace DSM::Geometry;

namespace DSM {
	Object::Object(const std::string& name) noexcept
		:m_Name(name) {
	}

	Object::Object(const std::string& name, const Model* model) noexcept
	:	m_Name(name){
		SetModel(model);
	}

	Object::~Object()
	{
		if (auto parent = m_Parent.lock();
			parent != nullptr && parent->m_ChildObject.contains(m_Name)) {
			parent->m_ChildObject.erase(m_Name);
		}
	}

	const std::string& Object::GetName() const noexcept
	{
		return m_Name;
	}

	Transform& Object::GetTransform() noexcept
	{
		return m_Transform;
	}

	const Transform& Object::GetTransform() const noexcept
	{
		return m_Transform;
	}

	const Model* Object::GetModel() const noexcept
	{
		return m_Model;
	}

	UINT Object::GetCBIndex() const noexcept
	{
		return m_ObjCBIndex;
	}

	DirectX::BoundingBox Object::GetBouningBox() const noexcept
	{
		return m_BoundingBox;
	}

	void Object::AddChild(std::shared_ptr<Object> child) noexcept
	{
		if (child == nullptr)return;

		// 若child的父物体为this则返回
		if (auto it = m_ChildObject.try_emplace(child->m_Name, child); !it.second){
			return;
		}
		
		// 若其有父物体则在父物体中移除该物体
		if (auto childParent = child->m_Parent.lock(); childParent != nullptr) {
			childParent->m_ChildObject.erase(child->m_Name);
		}
		child->m_Parent = weak_from_this();
	}

	void Object::SetName(const std::string& name)
	{
		m_Name = name;
	}

	void Object::SetModel(const Model* model) noexcept
	{
		m_Model = model;

		if (model != nullptr && !model->GetAllMesh().empty()){
			m_BoundingBox = BoundingBox{};
			for (const auto& [name,mesh] : model->GetAllMesh()){
				BoundingBox::CreateMerged(m_BoundingBox, m_BoundingBox, mesh.m_BoundingBox);
			}
		}
	}

	void Object::SetParent(std::shared_ptr<Object> parent) noexcept
	{
		if (parent == nullptr)return;
		
		parent->m_ChildObject[m_Name] = shared_from_this();
		
		if (auto preParent = m_Parent.lock(); preParent != nullptr) {
			preParent->m_ChildObject.erase(m_Name);
		}
		m_Parent = parent;
	}

	void Object::SetBoundingBox(const DirectX::BoundingBox& boundingBox) noexcept
	{
		m_BoundingBox = boundingBox;
	}

	void Object::SetCBIndex(UINT index) noexcept
	{
		m_ObjCBIndex = index;
	}
}
