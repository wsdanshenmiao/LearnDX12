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

	const Model* ModelManager::GetModel(const std::string& modelName)
	{
		if (auto it = m_Models.find(modelName); it != m_Models.end()) {
			return &it->second;
		}
		else {
			return nullptr;
		}
	}

	const std::unordered_map<std::string, Model>& ModelManager::GetAllModel()
	{
		return m_Models;
	}

	void ModelManager::ClearModels()
	{
		m_Models.clear();
	}
}
