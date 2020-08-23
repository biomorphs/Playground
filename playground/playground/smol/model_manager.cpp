#include "model_manager.h"
#include "model.h"
#include "texture_manager.h"
#include "sde/job_system.h"
#include "kernel/assert.h"
#include "core/profiler.h"
#include "core/scoped_mutex.h"
#include "kernel/thread.h"
#include "debug_gui/debug_gui_system.h"

namespace smol
{
	ModelManager::ModelManager(TextureManager* tm, SDE::JobSystem* js)
		: m_textureManager(tm)
		, m_jobSystem(js)
	{
	}

	bool ModelManager::ShowGui(DebugGui::DebugGuiSystem& gui)
	{
		static bool s_showWindow = true;
		gui.BeginWindow(s_showWindow, "ModelManager");
		char text[1024] = { '\0' };
		sprintf_s(text, "Loading: %d", m_inFlightModels.Get());
		gui.Text(text);
		gui.Separator();
		for (int t = 0; t < m_models.size(); ++t)
		{
			sprintf_s(text, "%d: %s (0x%p)", t, m_models[t].m_name.c_str(), m_models[t].m_model.get());
			gui.Text(text);
		}
		gui.EndWindow();
		return s_showWindow;
	}

	void ModelManager::ReloadAll()
	{
		SDE_PROF_EVENT();

		// wait until all jobs finish, not great but eh
		while (m_inFlightModels.Get() > 0)
		{
			int v = m_inFlightModels.Get();
			Kernel::Thread::Sleep(1);
		}
		// clear out the old results
		{
			Core::ScopedMutex lock(m_loadedModelsMutex);
			m_loadedModels.clear();
		}
		// now load the models again
		auto currentModels = std::move(m_models);
		for (int m = 0; m < currentModels.size(); ++m)
		{
			auto newHandle = LoadModel(currentModels[m].m_name.c_str());
			SDE_ASSERT(m == newHandle.m_index, "Bum");
		}
	}

	// this must be called on main thread before rendering!
	void ModelManager::FinaliseModel(Assets::Model& model, Model& renderModel, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders)
	{
		SDE_PROF_EVENT();
		const auto meshCount = renderModel.Parts().size();
		for (int index = 0; index < meshCount; ++index)
		{
			if (renderModel.Parts()[index].m_mesh != nullptr)
			{
				const auto& mesh = model.Meshes()[index];

				// Load all textures for each part
				std::string diffuseTexturePath = mesh.Material().DiffuseMaps().size() > 0 ? mesh.Material().DiffuseMaps()[0] : "white.bmp";
				std::string normalTexturePath = mesh.Material().NormalMaps().size() > 0 ? mesh.Material().NormalMaps()[0] : "";
				std::string specTexturePath = mesh.Material().SpecularMaps().size() > 0 ? mesh.Material().SpecularMaps()[0] : "";
				renderModel.Parts()[index].m_diffuse = m_textureManager->LoadTexture(diffuseTexturePath.c_str());
				renderModel.Parts()[index].m_normalMap = m_textureManager->LoadTexture(normalTexturePath.c_str());
				renderModel.Parts()[index].m_specularMap = m_textureManager->LoadTexture(specTexturePath.c_str());

				// Create render resources that cannot be shared across contexts
				meshBuilders[index]->CreateVertexArray(*renderModel.Parts()[index].m_mesh);
			}
		}
	}

	std::unique_ptr<Model> ModelManager::CreateModel(Assets::Model& model, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders)
	{
		char debugName[1024] = { '\0' };
		sprintf_s(debugName, "smol::ModelManager::ProcessModel(\"%s\")", model.GetPath().c_str());
		SDE_PROF_EVENT_DYN(debugName);

		auto resultModel = std::make_unique<Model>();
		const auto& builder = meshBuilders.begin();

		const auto meshCount = model.Meshes().size();
		for (int index=0;index<meshCount;++index)
		{
			const auto& mesh = model.Meshes()[index];

			auto newMesh = std::make_unique<Render::Mesh>();
			meshBuilders[index]->CreateMesh(*newMesh);

			Model::Part newPart;
			newPart.m_mesh = std::move(newMesh);
			newPart.m_transform = mesh.Transform();
			resultModel->Parts().push_back(std::move(newPart));
		}

		return resultModel;
	}

	void ModelManager::ProcessLoadedModels()
	{
		SDE_PROF_EVENT();
		
		std::vector<ModelLoadResult> loadedModels;
		{
			Core::ScopedMutex lock(m_loadedModelsMutex);
			loadedModels = std::move(m_loadedModels);
		}

		for (auto& loadedModel : loadedModels)
		{
			SDE_ASSERT(loadedModel.m_destinationHandle.m_index != -1, "Bad index");
			if (loadedModel.m_renderModel != nullptr)
			{
				FinaliseModel(*loadedModel.m_model, *loadedModel.m_renderModel, loadedModel.m_meshBuilders);
				m_models[loadedModel.m_destinationHandle.m_index].m_model = std::move(loadedModel.m_renderModel);
			}
		}
	}

	std::unique_ptr<Render::MeshBuilder> ModelManager::CreateBuilderForPart(const Assets::ModelMesh& mesh)
	{
		SDE_PROF_EVENT();

		auto builder = std::make_unique<Render::MeshBuilder>();
		builder->AddVertexStream(3, mesh.Indices().size());		// position
		builder->AddVertexStream(3, mesh.Indices().size());		// normal
		builder->AddVertexStream(3, mesh.Indices().size());		// tangents
		builder->AddVertexStream(2, mesh.Indices().size());		// uv
		builder->BeginChunk();
		const auto& vertices = mesh.Vertices();
		const auto& indices = mesh.Indices();
		{
			SDE_PROF_EVENT("SetStreamData");
			for (uint32_t index = 0; index < indices.size(); index += 3)
			{
				const auto& v0 = vertices[indices[index]];
				const auto& v1 = vertices[indices[index + 1]];
				const auto& v2 = vertices[indices[index + 2]];
				builder->BeginTriangle();
				builder->SetStreamData(0, v0.m_position, v1.m_position, v2.m_position);
				builder->SetStreamData(1, v0.m_normal, v1.m_normal, v2.m_normal);
				builder->SetStreamData(2, v0.m_tangent, v1.m_tangent, v2.m_tangent);
				builder->SetStreamData(3, v0.m_texCoord0, v1.m_texCoord0, v2.m_texCoord0);
				builder->EndTriangle();
			}
		}
		builder->EndChunk();
		return builder;
	}

	ModelHandle ModelManager::LoadModel(const char* path)
	{
		for (uint64_t i = 0; i < m_models.size(); ++i)
		{
			if (m_models[i].m_name == path)
			{
				return { static_cast<uint16_t>(i) };
			}
		}

		// always make a valid handle
		m_models.push_back({nullptr, path });
		auto newHandle = ModelHandle{ static_cast<uint16_t>(m_models.size() - 1) };
		m_inFlightModels.Add(1);

		std::string pathString = path;
		m_jobSystem->PushJob([this, pathString, newHandle]() {
			auto loadedAsset = Assets::Model::Load(pathString.c_str());
			if (loadedAsset != nullptr)
			{
				std::vector<std::unique_ptr<Render::MeshBuilder>> meshBuilders;
				for (const auto& part : loadedAsset->Meshes())
				{
					meshBuilders.push_back(CreateBuilderForPart(part));
				}

				auto theModel = CreateModel(*loadedAsset, meshBuilders);	// this does not create VAOs as they cannot be shared across contexts

				Core::ScopedMutex lock(m_loadedModelsMutex);
				m_loadedModels.push_back({ std::move(loadedAsset), std::move(theModel), std::move(meshBuilders), newHandle });
				m_inFlightModels.Add(-1);
			}
		});
		
		return newHandle;
	}

	Model* ModelManager::GetModel(const ModelHandle& h)
	{
		if (h.m_index != -1 && h.m_index < m_models.size())
		{
			return m_models[h.m_index].m_model.get();
		}
		else
		{
			return nullptr;
		}
	}
}