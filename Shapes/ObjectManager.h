#pragma once
#ifndef __OBJECTMANAGER__H__
#define __OBJECTMANAGER__H__

#include "Object.h"
#include "D3DUtil.h"
#include "Singleton.h"

namespace DSM {

	class StaticMeshManager : public Singleton<StaticMeshManager>
	{
	public:
		bool IsChange() const noexcept;
		void AddMesh(std::shared_ptr<Object> obj);

		std::size_t GetMeshSize() const noexcept;
		std::size_t GetObjectSize() const noexcept;
		std::shared_ptr<Object> GetMesh(const std::string& name);
		std::shared_ptr<Object> GetObjectByIndex(const std::size_t& index);
		Geometry::GeometryMesh GetAllObjectMesh() const;
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
		bool m_IsChange = false;
		std::vector<std::shared_ptr<Object>> m_Objects;
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

		GeometryMesh mesh = GetAllObjectMesh();
		UINT preIndexCount = 0;
		UINT preVertexCount = 0;
		for (std::size_t i = 0; i < m_Objects.size(); ++i) {
			auto& objectMesh = m_Objects[i]->GetRenderItem();
			if (!objectMesh) continue;
			SubmeshData submesh;
			submesh.m_Bound = m_Objects[i]->GetBouningBox();
			auto objIndexCount = (UINT)objectMesh->m_Indices32.size();
			submesh.m_IndexCount = objIndexCount;
			submesh.m_StarIndexLocation = preIndexCount;
			submesh.m_BaseVertexLocation = preVertexCount;
			preIndexCount += objIndexCount;
			preVertexCount += (UINT)objectMesh->m_Vertices.size();
			meshData->m_DrawArgs.insert(std::make_pair(m_Objects[i]->m_Name, std::move(submesh)));
		}

		meshData->m_IndexSize = (UINT)mesh.m_Indices32.size();
		std::vector<std::uint32_t> indices = mesh.m_Indices32;
		std::vector<VertexData> vertices;
		vertices.reserve(mesh.m_Vertices.size());
		for (const auto& vert : mesh.m_Vertices) {
			vertices.push_back(vertFunc(vert));
		}

		auto vbByteSize = vertices.size() * sizeof(VertexData);
		auto ibByteSize = indices.size() * sizeof(std::uint32_t);
		ThrowIfFailed(D3DCreateBlob(vbByteSize, &meshData->m_VertexBufferCPU));
		memcpy(meshData->m_VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &meshData->m_IndexBufferCPU));
		memcpy(meshData->m_IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		meshData->m_VertexBufferGPU = D3DUtil::CreateDefaultBuffer(device,
			cmdList, vertices.data(), vbByteSize, meshData->m_VertexBufferUploader);

		meshData->m_IndexBufferGPU = D3DUtil::CreateDefaultBuffer(device,
			cmdList, indices.data(), ibByteSize, meshData->m_IndexBufferUploader);

		meshData->m_VertexByteStride = (UINT)sizeof(VertexData);
		meshData->m_VertexBufferByteSize = (UINT)vbByteSize;
		meshData->m_IndexFormat = DXGI_FORMAT_R32_UINT;
		meshData->m_IndexBufferByteSize = (UINT)ibByteSize;

		return meshData;
	}


}

#endif // !__OBJECTMANAGER__H__
