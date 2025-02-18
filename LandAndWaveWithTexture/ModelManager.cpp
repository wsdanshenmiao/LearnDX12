#include "ModelManager.h"
#include "Vertex.h"

using namespace DSM::Geometry;
using namespace DirectX;

namespace DSM {
	const Model* ModelManager::LoadModelFromeFile(
		const std::string& name,
		const std::string& filename,
		ID3D12GraphicsCommandList* cmdList)
	{
		Model model;
		if (Model::LoadModelFromFile(model, name, filename, cmdList)){
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

	ModelManager::ModelManager(ID3D12Device* device)
		:m_Device(device){
	}
}
