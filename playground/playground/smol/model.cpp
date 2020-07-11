#include "model.h"
#include "../model_asset.h"
#include "render/mesh_builder.h"
#include "render/mesh.h"
#include "renderer.h"

namespace smol
{
	std::unique_ptr<Model> Model::CreateFromAsset(const Assets::Model& m, TextureManager& tm)
	{
		auto resultModel = std::make_unique<Model>();
		resultModel->m_parts.reserve(m.Meshes().size());
		for (const auto& mesh : m.Meshes())
		{
			Render::MeshBuilder builder;
			builder.AddVertexStream(3, mesh.Indices().size());		// position
			builder.AddVertexStream(2, mesh.Indices().size());		// uv
			builder.BeginChunk();
			const auto& vertices = mesh.Vertices();
			const auto& indices = mesh.Indices();
			for (uint32_t index = 0; index < indices.size(); index += 3)
			{
				const auto& v0 = vertices[indices[index]];
				const auto& v1 = vertices[indices[index + 1]];
				const auto& v2 = vertices[indices[index + 2]];
				builder.BeginTriangle();
				builder.SetStreamData(0, v0.m_position, v1.m_position, v2.m_position);
				builder.SetStreamData(1, v0.m_texCoord0, v1.m_texCoord0, v2.m_texCoord0);
				builder.EndTriangle();
			}
			builder.EndChunk();

			auto newMesh = new Render::Mesh();
			builder.CreateMesh(*newMesh);

			std::string diffuseTexturePath = mesh.Material().DiffuseMaps().size() > 0 ? mesh.Material().DiffuseMaps()[0] : "white.bmp";
			auto diffuseTexture = tm.LoadTexture(diffuseTexturePath.c_str());

			resultModel->m_parts.push_back({ newMesh, diffuseTexture, mesh.Transform() });
		}
		return resultModel;
	}
}