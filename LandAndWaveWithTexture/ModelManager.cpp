#include "ModelManager.h"
#include "Vertex.h"

using namespace DSM::Geometry;
using namespace DirectX;

namespace DSM {
	const Model* ModelManager::LoadModelFromeFile(const std::string& name, const std::string& filename)
	{
		Model model;
		if (Model::LoadModelFromFile(model, name, filename)){
			m_Models[name] = std::move(model);
			return &m_Models[name];
		}
		else{
			return nullptr;
		}
	}

	const Model* ModelManager::LoadModelFromeGeometry(
		const std::string& name,
		const Geometry::GeometryMesh& mesh)
	{
		Model model;
		if (Model::LoadModelFromeGeometry(model, name, mesh)){
			m_Models[name] = std::move(model);
			return &m_Models[name];
		}
		else{
			return nullptr;
		}
		
	}

	void ModelManager::ClearModels()
	{
		m_Models.clear();
	}
}
