#pragma once
#ifndef __STATICMESHMANAGER__H__
#define __STATICMESHMANAGER__H__

#include "D3DUtil.h"
#include "Singleton.h"
#include "Geometry.h"
#include "MeshData.h"
#include "Model.h"

namespace DSM {

	/// <summary>
	/// 统一管理模型，并生成其网格数据
	/// </summary>
	class ModelManager : public Singleton<ModelManager>
	{
	public:
		const Model* LoadModelFromeFile(const std::string& name, const std::string& filename);
		const Model* LoadModelFromeGeometry(
			const std::string& name,
			const Geometry::GeometryMesh& mesh);

		template<typename VertexData, typename VertFunc>
		const Geometry::MeshData* CreateMeshData(
			const std::string& modelName,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			VertFunc vertFunc);

		template<typename VertexData>
		const Geometry::MeshData* GetMeshData(const std::string& modelName);
			
		void ClearModels();

	private:
		friend Singleton<ModelManager>;
		ModelManager() = default;
		~ModelManager() = default;


	private:
		std::unordered_map<std::string, Model> m_Models;
		std::unordered_map<std::string, Geometry::MeshData> m_MeshDatas;
	};

	template<typename VertexData, typename VertFunc>
	inline const Geometry::MeshData* ModelManager::CreateMeshData(
		const std::string& modelName,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		VertFunc vertFunc)
	{
		using namespace DSM::Geometry;
		
		MeshData meshData{};
		auto meshDataName = modelName + typeid(VertexData).name(); 
		meshData.m_Name = meshDataName;

		// 查找是否已经生成了对应的网格数据s
		if (auto it = m_MeshDatas.find(meshData.m_Name); it != m_MeshDatas.end()){
			return &it->second;
		}

		// 查找是否有该模型
		auto modelData = m_Models.find(modelName);
		if (modelData == m_Models.end()){
			return nullptr;
		}

		const Model& model = modelData->second;

		GeometryMesh totalMesh{};
		auto& meshVertices = totalMesh.m_Vertices;
		auto& meshIndices = totalMesh.m_Indices32;

		UINT preIndexCount = 0;
		UINT preVertexCount = 0;
		for (const auto& [modelMeshName, modelMesh] : model.GetAllMesh()) {
			auto& mesh = modelMesh.m_Mesh;
			auto indexCount = (UINT)mesh.m_Indices32.size();
			SubmeshData submesh;
			submesh.m_Bound = modelMesh.m_BoundingBox;
			submesh.m_IndexCount = indexCount;
			submesh.m_StarIndexLocation = preIndexCount;
			submesh.m_BaseVertexLocation = preVertexCount;
			meshData.m_DrawArgs.insert(std::make_pair(modelMeshName, std::move(submesh)));
			
			preIndexCount += indexCount;
			preVertexCount += (UINT)mesh.m_Vertices.size();

			meshVertices.insert(meshVertices.end(), mesh.m_Vertices.begin(), mesh.m_Vertices.end());
			meshIndices.insert(meshIndices.end(), mesh.m_Indices32.begin(), mesh.m_Indices32.end());
		}
		
		meshData.CreateMeshData<VertexData, VertFunc>(totalMesh, device, cmdList, vertFunc);

		m_MeshDatas[meshDataName] = std::move(meshData);

		return &m_MeshDatas[meshDataName];
	}

	template<typename VertexData>
	inline const Geometry::MeshData* ModelManager::GetMeshData(const std::string& modelName)
	{
		if (auto it = m_MeshDatas.find(modelName + typeid(VertexData).name()); it != m_MeshDatas.end()) {
			return &it->second;
		}
		else {
			return nullptr;
		}
	}
	

}

#endif // !__STATICMESHMANAGER__H__
