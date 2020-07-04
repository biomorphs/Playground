#pragma once

#include "math/glm_headers.h"

#include <vector>
#include <string>
#include <memory>

class MeshVertex
{
public:
	glm::vec3 m_position;
	glm::vec3 m_normal;
	glm::vec2 m_texCoord0;
};

class MeshMaterial
{
public:
	MeshMaterial() = default;
	~MeshMaterial() = default;
	MeshMaterial(const MeshMaterial&) = delete;
	MeshMaterial(MeshMaterial&&) = default;

	std::vector<std::string>& DiffuseMaps() { return m_diffuseMaps; }
	const std::vector<std::string>& DiffuseMaps() const { return m_diffuseMaps; }

	std::vector<std::string>& NormalMaps() { return m_normalMaps; }
	const std::vector<std::string>& NormalMaps() const { return m_normalMaps; }

	std::vector<std::string>& SpecularMaps() { return m_specularMaps; }
	const std::vector<std::string>& SpecularMaps() const { return m_specularMaps; }

private:
	std::vector<std::string> m_diffuseMaps;
	std::vector<std::string> m_normalMaps;
	std::vector<std::string> m_specularMaps;
};

class ModelMesh
{
public:
	ModelMesh() = default;
	~ModelMesh() = default;
	ModelMesh(const ModelMesh&) = delete;
	ModelMesh(ModelMesh&&) = default;

	std::vector<MeshVertex>& Vertices() { return m_vertices; }
	const std::vector<MeshVertex>& Vertices() const { return m_vertices; }

	std::vector<uint32_t>& Indices() { return m_indices; }
	const std::vector<uint32_t>& Indices() const { return m_indices; }

private:
	std::vector<MeshVertex> m_vertices;
	std::vector<uint32_t> m_indices;
	MeshMaterial m_material;
};

// A render-agnostic model
class Model
{
public:
	Model() = default;
	~Model() = default;
	Model(const Model&) = delete;
	Model(Model&&) = default;

	class Loader
	{
	public:
		static std::unique_ptr<Model> Load(const char* path);
	private:
		static void ParseSceneNode(const struct aiScene* scene, const struct aiNode* node, Model& model);
		static void ProcessMesh(const aiScene* scene, const struct aiMesh* mesh, Model& model);
	};

private:
	std::vector<ModelMesh> m_meshes;
	std::string m_path;
};