#pragma once
#ifndef __OBJECT__H__
#define __OBJECT__H__

#include <memory>
#include <string>
#include <unordered_map>
#include <DirectXCollision.h>
#include "Transform.h"
#include "Geometry.h"
#include "Mesh.h"

namespace DSM {

	class Object
	{
	public:
		Object(const std::string& name) noexcept;
		Object(const std::string& name, std::shared_ptr< Geometry::GeometryMesh> geometryMesh) noexcept;
		~Object();
		Transform& GetTransform() noexcept;
		const Transform& GetTransform() const noexcept;
		DirectX::BoundingBox GetBouningBox() const noexcept;
		const std::shared_ptr<Geometry::GeometryMesh> GetRenderItem() const noexcept;

		void SetParent(Object* parent) noexcept;
		void SetBoundingBox(const DirectX::BoundingBox& boundingBox) noexcept;;
		void SetGeometryMesh(std::shared_ptr<Geometry::GeometryMesh> pMesh) noexcept;

		void AddChild(Object* child) noexcept;


	public:
		std::string m_Name;

	private:
		Transform m_Transform;
		std::shared_ptr<Geometry::GeometryMesh> m_GeometryMesh = nullptr;
		std::unordered_map<std::string, Object*> m_ChildObject;
		Object* m_Parent = nullptr;
		DirectX::BoundingBox m_BoundingBox;
	};
}

#endif // !__OBJECT__H__
