#pragma once
#ifndef __STATICMESHMANAGER__H__
#define __STATICMESHMANAGER__H__

#include "D3DUtil.h"
#include "Singleton.h"
#include "Geometry.h"
#include "Mesh.h"

namespace DSM {

	/// <summary>
	/// 统一管理静态网格，并生成所有网格的网格数据
	/// </summary>
	class StaticMeshManager : public Singleton<StaticMeshManager>
	{
	public:
		void AddMesh(
			const std::string& name,
			Geometry::GeometryMesh&& mesh,
			DirectX::BoundingBox box = DirectX::BoundingBox());

		std::size_t GetMeshSize() const noexcept;
		std::pair<Geometry::GeometryMesh, DirectX::BoundingBox> GetMesh(const std::string& name);
		template<typename VertexData, typename VertFunc>
		std::unique_ptr<Geometry::MeshData> GetAllMeshData(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			const std::string& name,
			VertFunc vertFunc) const;

	private:
		friend Singleton<StaticMeshManager>;
		StaticMeshManager() = default;
		~StaticMeshManager() = default;


	private:
		std::unordered_map<std::string, std::pair<Geometry::GeometryMesh, DirectX::BoundingBox>> m_GeometryMesh;
	};

	template<typename VertexData, typename VertFunc>
	std::unique_ptr<Geometry::MeshData> StaticMeshManager::GetAllMeshData(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const std::string& meshName,
		VertFunc vertFunc) const
	{
		using namespace DSM::Geometry;
		auto meshData = std::make_unique<MeshData>();
		meshData->m_Name = meshName;

		std::vector<Vertex> meshVertices;
		std::vector<std::uint32_t> meshIndices;

		UINT preIndexCount = 0;
		UINT preVertexCount = 0;
		for (const auto& meshMes : m_GeometryMesh) {
			SubmeshData submesh;
			auto& mesh = meshMes.second.first;
			submesh.m_Bound = meshMes.second.second;
			auto objIndexCount = (UINT)mesh.m_Indices32.size();
			submesh.m_IndexCount = objIndexCount;
			submesh.m_StarIndexLocation = preIndexCount;
			submesh.m_BaseVertexLocation = preVertexCount;
			preIndexCount += objIndexCount;
			preVertexCount += (UINT)mesh.m_Vertices.size();
			meshData->m_DrawArgs.insert(std::make_pair(meshMes.first, std::move(submesh)));

			meshVertices.insert(meshVertices.end(), mesh.m_Vertices.begin(), mesh.m_Vertices.end());
			meshIndices.insert(meshIndices.end(), mesh.m_Indices32.begin(), mesh.m_Indices32.end());
		}

		std::vector<VertexData> verticesData;
		verticesData.reserve(meshVertices.size());
		for (const auto& vert : meshVertices) {
			verticesData.push_back(vertFunc(vert));
		}

		auto vbByteSize = verticesData.size() * sizeof(VertexData);
		auto ibByteSize = meshIndices.size() * sizeof(std::uint32_t);
		ThrowIfFailed(D3DCreateBlob(vbByteSize, &meshData->m_VertexBufferCPU));
		memcpy(meshData->m_VertexBufferCPU->GetBufferPointer(), verticesData.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &meshData->m_IndexBufferCPU));
		memcpy(meshData->m_IndexBufferCPU->GetBufferPointer(), meshIndices.data(), ibByteSize);

		meshData->m_VertexBufferGPU = D3DUtil::CreateDefaultBuffer(device,
			cmdList, verticesData.data(), vbByteSize, meshData->m_VertexBufferUploader);

		meshData->m_IndexBufferGPU = D3DUtil::CreateDefaultBuffer(device,
			cmdList, meshIndices.data(), ibByteSize, meshData->m_IndexBufferUploader);

		meshData->m_VertexByteStride = (UINT)sizeof(VertexData);
		meshData->m_VertexBufferByteSize = (UINT)vbByteSize;
		meshData->m_IndexFormat = DXGI_FORMAT_R32_UINT;
		meshData->m_IndexBufferByteSize = (UINT)ibByteSize;

		return meshData;
	}


}

#endif // !__STATICMESHMANAGER__H__
