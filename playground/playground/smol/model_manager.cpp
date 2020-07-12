#include "model_manager.h"
#include "model.h"
#include "texture_manager.h"
#include "sde/job_system.h"
#include "kernel/assert.h"

namespace smol
{
	ModelManager::ModelManager(TextureManager* tm, SDE::JobSystem* js)
		: m_textureManager(tm)
		, m_jobSystem(js)
	{
	}

	void ModelManager::ProcessLoadedModels()
	{
		Kernel::ScopedMutex lock(m_loadedModelsMutex);
		for (auto& loadedModel : m_loadedModels)
		{
			SDE_ASSERT(loadedModel.m_destinationHandle.m_index != -1, "Bad index");

			if (loadedModel.m_model != nullptr)
			{
				auto renderModel = smol::Model::CreateFromAsset(*loadedModel.m_model, *m_textureManager);
				if (renderModel != nullptr)
				{
					m_models[loadedModel.m_destinationHandle.m_index].m_model = std::move(renderModel);
				}
			}
		}
		m_loadedModels.clear();
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

		std::string pathString = path;
		m_jobSystem->PushJob([this, pathString, newHandle]() {
			auto loadedAsset = Assets::Model::Load(pathString.c_str());
			if (loadedAsset != nullptr)
			{
				Kernel::ScopedMutex lock(m_loadedModelsMutex);
				m_loadedModels.push_back({ std::move(loadedAsset), newHandle });
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