#include "texture_manager.h"

#include "sde/job_system.h"
#include "../stb_image.h"
#include "core/profiler.h"
#include "core/scoped_mutex.h"

namespace smol
{
	TextureManager::TextureManager(SDE::JobSystem* js)
		: m_jobSystem(js)
	{
	}

	void TextureManager::ProcessLoadedTextures()
	{
		SDE_PROF_EVENT();

		std::vector<LoadedTexture> loadedTextures;
		{
			SDE_PROF_EVENT("AcquireLoadedTextures");
			Core::ScopedMutex lock(m_loadedTexturesMutex);
			loadedTextures = std::move(m_loadedTextures);
		}

		{
			SDE_PROF_EVENT("CreateTextures");
			for (const auto& src : loadedTextures)
			{
				auto newTex = std::make_unique<Render::Texture>();
				if (newTex->Create(src.m_texture))
				{
					m_textures[src.m_destination.m_index].m_texture = std::move(newTex);
				}
			}
		}
	}

	TextureHandle TextureManager::LoadTexture(const char* path)
	{
		for (uint64_t i = 0; i < m_textures.size(); ++i)
		{
			if (m_textures[i].m_path == path)
			{
				return { static_cast<uint16_t>(i) };
			}
		}

		m_textures.push_back({nullptr, path });
		auto newHandle = TextureHandle{ static_cast<uint16_t>(m_textures.size() - 1) };

		std::string pathString = path;
		m_jobSystem->PushJob([this, pathString, newHandle]() {
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "LoadTexture(\"%s\")", pathString.c_str());
			SDE_PROF_EVENT_DYN(debugName);

			int w, h, components;
			stbi_set_flip_vertically_on_load(true);
			unsigned char* loadedData = stbi_load(pathString.c_str(), &w, &h, &components, 4);		// we always want rgba
			if (loadedData == nullptr)
			{
				return;
			}

			std::vector<uint8_t> rawDataBuffer;
			rawDataBuffer.resize(w * h * 4);
			memcpy(rawDataBuffer.data(), loadedData, w * h * 4);
			stbi_image_free(loadedData);

			std::vector<Render::TextureSource::MipDesc> mip;
			mip.push_back({ (uint32_t)w,(uint32_t)h,0,w * h * (size_t)4 });
			Render::TextureSource ts((uint32_t)w, (uint32_t)h, Render::TextureSource::Format::RGBA8, { mip }, rawDataBuffer);

			{
				SDE_PROF_EVENT("PushToResultsList");
				Core::ScopedMutex lock(m_loadedTexturesMutex);
				{
					SDE_PROF_EVENT("PushBack");
					m_loadedTextures.push_back({ std::move(ts), newHandle });
				}
			}
		});

		return newHandle;
	}

	Render::Texture* TextureManager::GetTexture(const TextureHandle& h)
	{
		if (h.m_index != -1 && h.m_index < m_textures.size())
		{
			return m_textures[h.m_index].m_texture.get();
		}
		else
		{
			return nullptr;
		}
	}

	TextureManager::~TextureManager()
	{
		for (auto& it : m_textures)
		{
			it.m_texture->Destroy();
		}
	}

}