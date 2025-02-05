#include "Model.h"
#include "Mesh.h"

using namespace Assimp;
using namespace DSM::Geometry;
using namespace DirectX;

namespace DSM {
	Model::Model(const std::string& name, const std::string& filename)
		:m_Name(name) {
		LoadModelFromFile(filename);
	}

	std::string& Model::GetName()
	{
		return m_Name;
	}

	const std::string& Model::GetName() const
	{
		return m_Name;
	}

	const ModelMesh& Model::GetMesh(std::size_t index) const
	{
		return m_Meshs[index];
	}

	const std::vector<ModelMesh>& Model::GetAllMesh() const
	{
		return m_Meshs;
	}

	const Material& Model::GetMaterial(std::size_t index) const
	{
		return m_Materials[index];
	}

	bool Model::LoadModelFromFile(const std::string& filename)
	{
		Importer importer;
		const aiScene* pScene = importer.ReadFile(
			filename,
			aiProcess_ConvertToLeftHanded |     // 转为左手系
			aiProcess_GenBoundingBoxes |        // 获取碰撞盒
			aiProcess_Triangulate |             // 将多边形拆分
			aiProcess_ImproveCacheLocality |    // 改善缓存局部性
			aiProcess_SortByPType);             // 按图元顶点数排序用于移除非三角形图元

		if (nullptr == pScene || !pScene->HasMeshes()) {
			std::string warning = "[Warning]: Failed to load \"";
			warning += filename;
			warning += "\"\n";
			OutputDebugStringA(warning.c_str());
			return false;
		}

		ProcessNode(pScene->mRootNode, pScene);

		m_Materials.resize(pScene->mNumMaterials);
		for (UINT i = 0; i < pScene->mNumMaterials; ++i) {
			auto& material = pScene->mMaterials[i];

			XMFLOAT3 vector{};
			float value{};
			uint32_t num = 3;
			aiString name;

			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_NAME, name))
				m_Materials[i].Set("Name", std::string{ name.C_Str() });
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, (float*)&vector, &num))
				m_Materials[i].Set("AmbientColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, (float*)&vector, &num))
				m_Materials[i].Set("DiffuseColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, (float*)&vector, &num))
				m_Materials[i].Set("SpecularColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_SPECULAR_FACTOR, value))
				m_Materials[i].Set("SpecularFactor", value);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, (float*)&vector, &num))
				m_Materials[i].Set("EmissiveColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_OPACITY, value))
				m_Materials[i].Set("Opacity", value);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_TRANSPARENT, (float*)&vector, &num))
				m_Materials[i].Set("TransparentColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_REFLECTIVE, (float*)&vector, &num))
				m_Materials[i].Set("ReflectiveColor", vector);
		}

		return true;

	}

	void Model::ProcessNode(aiNode* node, const aiScene* scene)
	{
		// 导入当前节点的网格
		for (UINT i = 0; i < node->mNumMeshes; ++i) {
			auto mesh = scene->mMeshes[node->mMeshes[i]];
			m_Meshs.push_back(ProcessMesh(mesh, scene));
		}

		// 导入子节点的网格
		for (UINT i = 0; i < node->mNumChildren; ++i) {
			ProcessNode(node->mChildren[i], scene);
		}
	}

	ModelMesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
	{
		ModelMesh modelMesh{};
		auto& geoMesh = modelMesh.m_MeshData;
		auto& vertices = geoMesh.m_Vertices;
		auto& indices = geoMesh.m_Indices32;
		modelMesh.m_Name = mesh->mName.C_Str();

		// 获取顶点数据
		vertices.reserve(mesh->mNumVertices);
		for (UINT i = 0; i < mesh->mNumVertices; ++i) {
			Vertex vertex{};
			if (mesh->HasPositions()) {
				const auto& pos = mesh->mVertices[i];
				vertex.m_Position = XMFLOAT3(pos.x, pos.y, pos.z);
			}
			if (mesh->HasNormals()) {
				const auto& normal = mesh->mNormals[i];
				vertex.m_Normal = XMFLOAT3(normal.x, normal.y, normal.z);
			}
			if (mesh->HasTangentsAndBitangents()) {
				const auto& tangent = mesh->mTangents[i];
				const auto& biTangent = mesh->mBitangents[i];
				vertex.m_Tangent = XMFLOAT4{ tangent.x,tangent.y,tangent.z,1 };
				vertex.m_BiTangent = XMFLOAT3{ biTangent.x,biTangent.y,biTangent.z };
			}
			// 目前只获取主纹理坐标
			if (mesh->HasTextureCoords(0)) {
				const auto& texCoord = mesh->mTextureCoords[0][i];
				vertex.m_TexCoord = XMFLOAT2{ texCoord.x,texCoord.y };
			}
			vertices.push_back(std::move(vertex));
		}

		// 获取索引
		auto numIndex = mesh->mFaces->mNumIndices;
		indices.resize(mesh->mNumFaces * numIndex);
		for (size_t i = 0; i < mesh->mNumFaces; ++i) {
			memcpy(indices.data() + i * numIndex, mesh->mFaces[i].mIndices, sizeof(uint32_t) * numIndex);
		}

		const auto& AABB = mesh->mAABB;
		BoundingBox::CreateFromPoints(
			modelMesh.m_BoundingBox,
			XMVECTOR{ AABB.mMin.x, AABB.mMin.y, AABB.mMin.z, 1 },
			XMVECTOR{ AABB.mMax.x, AABB.mMax.y, AABB.mMax.z, 1 });

		modelMesh.m_MaterialIndex = mesh->mMaterialIndex;

		return modelMesh;
	}


}