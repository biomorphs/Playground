#include "model.h"
#include "../model_asset.h"
#include "render/mesh_builder.h"
#include "render/mesh.h"
#include "renderer.h"
#include "core/profiler.h"

namespace smol
{
	std::unique_ptr<Model> Model::CreateFromAsset(const Assets::Model& m, TextureManager& tm)
	{
		SDE_PROF_EVENT();
		OPTICK_TAG("Path", m.GetPath().c_str());

		auto resultModel = std::make_unique<Model>();
		resultModel->m_parts.reserve(m.Meshes().size());
		for (const auto& mesh : m.Meshes())
		{
			SDE_PROF_EVENT("BuildMesh");
			Render::MeshBuilder builder;
			builder.AddVertexStream(3, mesh.Indices().size());		// position
			builder.AddVertexStream(3, mesh.Indices().size());		// normal
			builder.AddVertexStream(3, mesh.Indices().size());		// tangents
			builder.AddVertexStream(2, mesh.Indices().size());		// uv
			builder.BeginChunk();
			const auto& vertices = mesh.Vertices();
			const auto& indices = mesh.Indices();
			{
				SDE_PROF_EVENT("SetStreamData");
				for (uint32_t index = 0; index < indices.size(); index += 3)
				{
					const auto& v0 = vertices[indices[index]];
					const auto& v1 = vertices[indices[index + 1]];
					const auto& v2 = vertices[indices[index + 2]];
					builder.BeginTriangle();
					builder.SetStreamData(0, v0.m_position, v1.m_position, v2.m_position);
					builder.SetStreamData(1, v0.m_normal, v1.m_normal, v2.m_normal);
					builder.SetStreamData(2, v0.m_tangent, v1.m_tangent, v2.m_tangent);
					builder.SetStreamData(3, v0.m_texCoord0, v1.m_texCoord0, v2.m_texCoord0);
					builder.EndTriangle();
				}
			}
			builder.EndChunk();

			auto newMesh = new Render::Mesh();
			builder.CreateMesh(*newMesh);

			std::string diffuseTexturePath = mesh.Material().DiffuseMaps().size() > 0 ? mesh.Material().DiffuseMaps()[0] : "white.bmp";
			std::string normalTexturePath = mesh.Material().NormalMaps().size() > 0 ? mesh.Material().NormalMaps()[0] : "";
			std::string specTexturePath = mesh.Material().SpecularMaps().size() > 0 ? mesh.Material().SpecularMaps()[0] : "";
			auto diffuseTexture = tm.LoadTexture(diffuseTexturePath.c_str());
			auto normalTexture = tm.LoadTexture(normalTexturePath.c_str());
			auto specularTexture = tm.LoadTexture(specTexturePath.c_str());
			resultModel->m_parts.push_back({ newMesh, diffuseTexture, normalTexture, specularTexture, mesh.Transform() });
		}
		return resultModel;
	}
}