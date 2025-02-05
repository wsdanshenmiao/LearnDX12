#include "MeshManager.h"
#include "Vertex.h"

using namespace DSM::Geometry;
using namespace DirectX;

namespace DSM {

	void StaticMeshManager::AddMesh(
		const std::string& name,
		GeometryMesh&& mesh,
		DirectX::BoundingBox box) {
		m_GeometryMesh.insert(std::make_pair(name, std::make_pair(mesh, box)));
	}

	std::size_t StaticMeshManager::GetMeshSize() const noexcept
	{
		return m_GeometryMesh.size();
	}

	std::pair<GeometryMesh, BoundingBox> StaticMeshManager::GetMesh(const std::string& name)
	{
		return m_GeometryMesh[name];
	}

}
