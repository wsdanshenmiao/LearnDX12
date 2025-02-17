#pragma once
#ifndef __MODEL__H__
#define __MODEL__H__

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
		Model() = default;
		Model(const std::string& name);
		Model(const std::string& name, const std::string& fileName);
		Model& operator= (const Model& other) = default;

		std::string& GetName();
		const std::string& GetName() const;
		const ModelMesh* GetMesh(const std::string& name) const;
		const std::map<std::string, ModelMesh>& GetAllMesh() const;
		const Material& GetMaterial(std::size_t index) const;

		void SetName(const std::string& name);
		void SetMesh(const ModelMesh& mesh);
		void SetMaterial(std::size_t index, const Material& material);

		void ClearMesh();
		void ClearMaterial();
		
		static bool LoadModelFromFile(Model& model, const std::string& name, const std::string& filename);
		static bool LoadModelFromeGeometry(Model& model, const std::string& name,const Geometry::GeometryMesh& mesh);
		
	private:
		static void ProcessNode(Model& model, aiNode* node, const aiScene* scene);
		static ModelMesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

	private:
		std::string m_Name;
		std::map<std::string, ModelMesh> m_Meshs;
		std::vector<Material> m_Materials;
	};

}

#endif // !__MODEL__H__
