#pragma once
#include "model.h"
#include "kernel/mutex.h"
#include "../model_asset.h"
#include <string>
#include <vector>
#include <memory>

namespace SDE
{
	class JobSystem;
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

	private:
		struct ModelDesc {
			std::unique_ptr<Model> m_model;
			std::string m_name;
		};
		std::vector<ModelDesc> m_models;
		
		struct ModelLoadResult {
			std::unique_ptr<Assets::Model> m_model;
			ModelHandle m_destinationHandle;
		};
		Kernel::Mutex m_loadedModelsMutex;
		std::vector<ModelLoadResult> m_loadedModels;	// models to process after successful load

		TextureManager* m_textureManager;
		SDE::JobSystem* m_jobSystem;
	};
}