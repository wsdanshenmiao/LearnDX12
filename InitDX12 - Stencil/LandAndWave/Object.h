#pragma once
#ifndef __OBJECT__H__
#define __OBJECT__H__

#include <memory>
#include <string>
#include <unordered_map>
#include <DirectXCollision.h>
#include "Transform.h"
#include "RenderItem.h"

namespace DSM {

	/// <summary>
	/// 一个物体对象
	/// </summary>
	class Object
	{
	public:
		Object() = default;
		Object(const std::string& name) noexcept;
		~Object();
		Transform& GetTransform() noexcept;
		const Transform& GetTransform() const noexcept;
		DirectX::BoundingBox GetBouningBox() const noexcept;
		const std::shared_ptr<RenderItem> GetRenderItem(const std::string& name) const noexcept;
		const std::vector<std::shared_ptr<RenderItem>>& GetAllRenderItems() const noexcept;
		std::size_t GetRenderItemsCount() const noexcept;

		void SetParent(Object* parent) noexcept;
		void SetBoundingBox(const DirectX::BoundingBox& boundingBox) noexcept;;
		void AddRenderItem(std::shared_ptr<RenderItem> item) noexcept;

		void AddChild(Object* child) noexcept;


	public:
		std::string m_Name;

	private:
		Transform m_Transform;
		std::vector<std::shared_ptr<RenderItem>> m_RenderItems;
		std::unordered_map<std::string, Object*> m_ChildObject;
		Object* m_Parent = nullptr;
		DirectX::BoundingBox m_BoundingBox;
	};
}

#endif // !__OBJECT__H__
