#include "Model.h"
#include "Texture.h"
#include <filesystem>

#include "TextureManager.h"

using namespace Assimp;
using namespace DSM::Geometry;
using namespace DirectX;

namespace DSM {
	Model::Model(const std::string& name)
		:m_Name(name){
	}
	
	std::string& Model::GetName()
	{
		return m_Name;
	}

	const std::string& Model::GetName() const
	{
		return m_Name;
	}

	const ModelMesh* Model::GetMesh(const std::string& name) const
	{
		auto it = m_Meshs.find(name);
		if (it == m_Meshs.end()){
			return nullptr;
		}
		else{
			return &it->second;
		}
	}

	const std::map<std::string, ModelMesh>& Model::GetAllMesh() const
	{
		return m_Meshs;
	}

	const Material& Model::GetMaterial(std::size_t index) const
	{
		return m_Materials[index];
	}

	void Model::SetName(const std::string& name)
	{
		m_Name = name;
	}

	void Model::SetMesh(const ModelMesh& mesh)
	{
		m_Meshs[mesh.m_Name] = mesh;
	}

	void Model::SetMaterial(std::size_t index, const Material& material)
	{
		if (index >= m_Materials.size()){
			m_Materials.resize(index + 1);
		}

		m_Materials[index] = material;
	}

	void Model::ClearMesh()
	{
		m_Meshs.clear();
	}

	void Model::ClearMaterial()
	{
		m_Materials.clear();
	}

	bool Model::LoadModelFromFile(
		Model& model,
		const std::string& name,
		const std::string& filename,
		ID3D12GraphicsCommandList* cmdList)
	{
		model.SetName(name);
		
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

		ProcessNode(model, pScene->mRootNode, pScene);
		ProcessMaterial(model, filename, cmdList, pScene);
		

		return true;

	}

	bool Model::LoadModelFromeGeometry(
		Model& model,
		const std::string& name,
		const Geometry::GeometryMesh& mesh)
	{
		if (mesh.m_Vertices.empty() || mesh.m_Indices32.empty()){
			return false;
		}

		model.ClearMesh();
		model.ClearMaterial();
		
		ModelMesh modelMesh;
		modelMesh.m_Mesh = mesh;
		modelMesh.m_Name = name;
		modelMesh.m_MaterialIndex = 0;
		modelMesh.m_BoundingBox = BoundingBox{};

		model.SetName(name);
		model.SetMesh(modelMesh);
		model.SetMaterial(0, Material::GetDefaultMaterial());

		return true;
	}

	void Model::ProcessNode(Model& model, aiNode* node, const aiScene* scene)
	{
		// 导入当前节点的网格
		for (UINT i = 0; i < node->mNumMeshes; ++i) {
			auto mesh = scene->mMeshes[node->mMeshes[i]];
			model.m_Meshs[std::string{mesh->mName.C_Str()}] = ProcessMesh(mesh);
		}

		// 导入子节点的网格
		for (UINT i = 0; i < node->mNumChildren; ++i) {
			ProcessNode(model, node->mChildren[i], scene);
		}
	}

	ModelMesh Model::ProcessMesh(aiMesh* mesh)
	{
		ModelMesh modelMesh{};
		auto& geoMesh = modelMesh.m_Mesh;
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

	void Model::ProcessMaterial(
		Model& model,
		const std::string& filename,
		ID3D12GraphicsCommandList* cmdList,
		const aiScene* scene)
	{
		model.m_Materials.resize(scene->mNumMaterials);
		for (UINT i = 0; i < scene->mNumMaterials; ++i) {
			auto& material = scene->mMaterials[i];

			XMFLOAT3 vector{};
			float value{};
			uint32_t num = 3;
			aiString matName;

			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_NAME, matName))
				model.m_Materials[i].Set("Name", std::string{ matName.C_Str() });
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, (float*)&vector, &num))
				model.m_Materials[i].Set("AmbientColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, (float*)&vector, &num))
				model.m_Materials[i].Set("DiffuseColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, (float*)&vector, &num))
				model.m_Materials[i].Set("SpecularColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_SPECULAR_FACTOR, value))
				model.m_Materials[i].Set("SpecularFactor", value);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, (float*)&vector, &num))
				model.m_Materials[i].Set("EmissiveColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_OPACITY, value))
				model.m_Materials[i].Set("Opacity", value);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_TRANSPARENT, (float*)&vector, &num))
				model.m_Materials[i].Set("TransparentColor", vector);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_REFLECTIVE, (float*)&vector, &num))
				model.m_Materials[i].Set("ReflectiveColor", vector);

			aiString aiPath;
			std::filesystem::path texFilename;
			std::string texName;

			auto tryCreateTexture = [&](aiTextureType type, const std::string& propertyName) {
				if (!material->GetTextureCount(type))
					return;

				material->GetTexture(type, 0, &aiPath);

				auto& texManager = TextureManager::GetInstance();

				// 纹理已经预先加载进来
				if (aiPath.data[0] == '*'){
					texName = filename;
					texName += aiPath.C_Str();
					char* pEndStr = nullptr;
					aiTexture* pTex = scene->mTextures[strtol(aiPath.data + 1, &pEndStr, 10)];
					texManager.LoadTextureFromMemory(
						texName,
						pTex->pcData,
						pTex->mHeight ? pTex->mWidth * pTex->mHeight : pTex->mWidth,
						cmdList);
					model.m_Materials[i].Set(propertyName, std::string(texName));
				}
				// 纹理通过文件名索引
				else{
					texFilename = filename;
					texFilename = texFilename.parent_path() / aiPath.C_Str();
					texManager.LoadTextureFromFile(texFilename.string(), cmdList);
					model.m_Materials[i].Set(propertyName, texFilename.string());
				}
			};

			tryCreateTexture(aiTextureType_DIFFUSE, "Diffuse");
			tryCreateTexture(aiTextureType_NORMALS, "Normal");
			tryCreateTexture(aiTextureType_BASE_COLOR, "Albedo");
			tryCreateTexture(aiTextureType_NORMAL_CAMERA, "NormalCamera");
			tryCreateTexture(aiTextureType_METALNESS, "Metalness");
			tryCreateTexture(aiTextureType_DIFFUSE_ROUGHNESS, "Roughness");
			tryCreateTexture(aiTextureType_AMBIENT_OCCLUSION, "AmbientOcclusion");
		}
	}
	
}

