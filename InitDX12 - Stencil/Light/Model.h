#pragma once
#ifndef __MODEL__H__
#define __MODEL__H__

#include "Mesh.h"
#include "Geometry.h"
#include "Material.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace DSM {

	struct ModelMesh
	{
		std::string m_Name;
		Geometry::GeometryMesh m_Mesh;
		DirectX::BoundingBox m_BoundingBox;
		UINT m_MaterialIndex;
	};

	class Model
	{
	public:
		Model(const std::string& name, const std::string& filename);

		std::string& GetName();
		const std::string& GetName() const;
		const ModelMesh& GetMesh(std::size_t index) const;
		const std::vector<ModelMesh>& GetAllMesh() const;
		const Material& GetMaterial(std::size_t index) const;

	private:
		bool LoadModelFromFile(const std::string& filename);
		void ProcessNode(aiNode* node, const aiScene* scene);
		ModelMesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

	private:
		std::string m_Name;
		std::vector<ModelMesh> m_Meshs;
		std::vector<Material> m_Materials;
	};

}

#endif // !__MODEL__H__
