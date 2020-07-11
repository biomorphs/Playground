#pragma once
#include "model.h"
#include <string>
#include <vector>
#include <memory>

namespace smol
{
	class TextureManager;

	struct ModelHandle
	{
		uint64_t m_index;
		static ModelHandle Invalid() { return { (uint64_t)-1 }; };
	};

	class ModelManager
	{
	public:
		ModelManager(TextureManager* tm);
		~ModelManager() = default;
		ModelManager(const ModelManager&) = delete;
		ModelManager(ModelManager&&) = delete;

		ModelHandle LoadModel(const char* path);
		Model* GetModel(const ModelHandle& h);

	private:
		struct ModelDesc {
			std::unique_ptr<Model> m_model;
			std::string m_name;
		};
		std::vector<ModelDesc> m_models;
		TextureManager* m_textureManager;
	};
}