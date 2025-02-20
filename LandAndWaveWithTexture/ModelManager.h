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
		const Model* LoadModelFromeFile(
			const std::string& name,
			const std::string& filename,
			ID3D12GraphicsCommandList* cmdList);
		const Model* LoadModelFromeGeometry(
			const std::string& name,
			const Geometry::GeometryMesh& mesh);

		template<typename VertexData, typename VertFunc>
		const Geometry::MeshData* CreateMeshData(
			const std::string& modelName,
			ID3D12GraphicsCommandList* cmdList,
			VertFunc vertFunc);
		template<typename VertexData, typename VertFunc>
		void CreateMeshDataForAllModel(
			ID3D12GraphicsCommandList* cmdList,
			VertFunc vertFunc);

		template<typename VertexData>
		Geometry::MeshData* GetMeshData(const std::string& modelName);
		const Model* GetModel(const std::string& modelName) const;
		Model* GetModel(const std::string& modelName);
		const std::unordered_map<std::string, Model>& GetAllModel();
			
		void ClearModels();

	private:
		friend Singleton<ModelManager>;
		ModelManager(ID3D12Device* device);
		~ModelManager() = default;


	private:
		Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
		std::unordered_map<std::string, Model> m_Models;
		std::unordered_map<std::string, Geometry::MeshData> m_MeshDatas;
	};

	template<typename VertexData, typename VertFunc>
	inline const Geometry::MeshData* ModelManager::CreateMeshData(
		const std::string& modelName,
		ID3D12GraphicsCommandList* cmdList,
		VertFunc vertFunc)
	{
		using namespace DSM::Geometry;
		
		auto meshDataName = modelName + typeid(VertexData).name(); 
		// 查找是否已经生成了对应的网格数据
		if (auto it = m_MeshDatas.find(meshDataName); it != m_MeshDatas.end()){
			return &it->second;
		}

		// 查找是否有该模型
		auto modelData = m_Models.find(modelName);
		if (modelData == m_Models.end()){
			return nullptr;
		}
		MeshData meshData{};
		meshData.m_Name = meshDataName;

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
		
		meshData.CreateMeshData<VertexData>(totalMesh, m_Device.Get(), cmdList, vertFunc);

		m_MeshDatas[meshDataName] = std::move(meshData);

		return &m_MeshDatas[meshDataName];
	}

	template <typename VertexData, typename VertFunc>
	inline void ModelManager::CreateMeshDataForAllModel(
		ID3D12GraphicsCommandList* cmdList,
		VertFunc vertFunc)
	{
		for (auto& [modelName, model] : m_Models) {
			CreateMeshData<VertexData, VertFunc>(modelName, cmdList, vertFunc);
		}
	}

	template<typename VertexData>
	inline Geometry::MeshData* ModelManager::GetMeshData(const std::string& modelName)
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
