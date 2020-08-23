#pragma once
#include "model.h"
#include "kernel/mutex.h"
#include "../model_asset.h"
#include "render/mesh_builder.h"
#include <string>
#include <vector>
#include <memory>

namespace SDE
{
	class JobSystem;
}

namespace DebugGui
{
	class DebugGuiSystem;
}

namespace smol
{
	class TextureManager;

	struct ModelHandle
	{
		uint16_t m_index = -1;
		static ModelHandle Invalid() { return { (uint16_t)-1 }; };
	};

	class ModelManager
	{
	public:
		ModelManager(TextureManager* tm, SDE::JobSystem* js);
		~ModelManager() = default;
		ModelManager(const ModelManager&) = delete;
		ModelManager(ModelManager&&) = delete;

		ModelHandle LoadModel(const char* path);
		Model* GetModel(const ModelHandle& h);
		void ProcessLoadedModels();

		bool ShowGui(DebugGui::DebugGuiSystem& gui);

		void ReloadAll();

	private:
		struct ModelDesc 
		{
			std::unique_ptr<Model> m_model;
			std::string m_name;
		};
		struct ModelLoadResult
		{
			std::unique_ptr<Assets::Model> m_model;
			std::unique_ptr<Model> m_renderModel;
			std::vector<std::unique_ptr<Render::MeshBuilder>> m_meshBuilders;
			ModelHandle m_destinationHandle;
		};
		std::unique_ptr<Render::MeshBuilder> CreateBuilderForPart(const Assets::ModelMesh&);
		std::unique_ptr<Model> CreateModel(Assets::Model& model, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders);
		void FinaliseModel(Assets::Model& model, Model& renderModel, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders);

		std::vector<ModelDesc> m_models;
	
		Kernel::Mutex m_loadedModelsMutex;
		std::vector<ModelLoadResult> m_loadedModels;	// models to process after successful load
		Kernel::AtomicInt32 m_inFlightModels = 0;

		TextureManager* m_textureManager;
		SDE::JobSystem* m_jobSystem;
	};
}