#include "model_manager.h"
#include "model.h"
#include "texture_manager.h"
#include "../model_asset.h"

namespace smol
{
	ModelManager::ModelManager(TextureManager* tm)
		: m_textureManager(tm)
	{
	}

	ModelHandle ModelManager::LoadModel(const char* path)
	{
		for (uint64_t i = 0; i < m_models.size(); ++i)
		{
			if (m_models[i].m_name == path)
			{
				return { i };
			}
		}

		auto loadedAsset = Assets::Model::Load(path);
		if (loadedAsset != nullptr)
		{
			auto renderModel = smol::Model::CreateFromAsset(*loadedAsset, *m_textureManager);
			if (renderModel != nullptr)
			{
				m_models.push_back({std::move(renderModel), path});
				return { m_models.size() - 1 };
			}
		}
		return ModelHandle::Invalid();
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