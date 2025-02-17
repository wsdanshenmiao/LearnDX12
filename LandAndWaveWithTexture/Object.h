#pragma once
#ifndef __OBJECT__H__
#define __OBJECT__H__

#include <memory>
#include <string>
#include <DirectXCollision.h>
#include "Transform.h"
#include "Model.h"

namespace DSM {

	/// <summary>
	/// 一个物体对象
	/// </summary>
	class Object :public std::enable_shared_from_this<Object>
	{
	public:
		Object() = default;
		Object(const std::string& name) noexcept;
		Object(const std::string& name, const Model* model)noexcept;
		~Object();

		const std::string& GetName() const noexcept;
		Transform& GetTransform() noexcept;
		const Transform& GetTransform() const noexcept;
		DirectX::BoundingBox GetBouningBox() const noexcept;
		const Model* GetModel() const noexcept;

		void SetName(const std::string& name);
		void SetModel(const Model* model) noexcept;
		void SetParent(std::shared_ptr<Object> parent) noexcept;
		void SetBoundingBox(const DirectX::BoundingBox& boundingBox) noexcept;;

		void AddChild(std::shared_ptr<Object> child) noexcept;
	
	private:
		std::string m_Name;
		Transform m_Transform;
		const Model* m_Model = nullptr;
		std::map<std::string, std::shared_ptr<Object>> m_ChildObject;
		std::weak_ptr<Object> m_Parent;
		DirectX::BoundingBox m_BoundingBox;
	};
}

#endif // !__OBJECT__H__
