#include "texture_manager.h"
#include "render/texture_source.h"
#include "../stb_image.h"

namespace smol
{
	TextureHandle TextureManager::LoadTexture(const char* path)
	{
		for (uint64_t i = 0; i < m_textures.size(); ++i)
		{
			if (m_textures[i].m_path == path)
			{
				return { i };
			}
		}

		auto newTex = Render::Texture();
		int w, h, components;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* loadedData = stbi_load(path, &w, &h, &components, 4);		// we always want rgba
		if (loadedData == nullptr)
		{
			stbi_image_free(loadedData);
			return TextureHandle::Invalid();
		}

		std::vector<uint8_t> rawDataBuffer;
		rawDataBuffer.resize(w * h * 4);
		memcpy(rawDataBuffer.data(), loadedData, w * h * 4);
		stbi_image_free(loadedData);

		std::vector<Render::TextureSource::MipDesc> mip;
		mip.push_back({ (uint32_t)w,(uint32_t)h,0,w * h * (size_t)4 });
		Render::TextureSource ts((uint32_t)w, (uint32_t)h, Render::TextureSource::Format::RGBA8, { mip }, rawDataBuffer);
		std::vector< Render::TextureSource> sources;
		sources.push_back(ts);
		if (!newTex.Create(sources))
		{
			return TextureHandle::Invalid();
		}

		m_textures.push_back({ std::move(newTex), path });
		return { m_textures.size() - 1 };
	}

	Render::Texture* TextureManager::GetTexture(const TextureHandle& h)
	{
		if (h.m_index != -1 && h.m_index < m_textures.size())
		{
			return &m_textures[h.m_index].m_texture;
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
			it.m_texture.Destroy();
		}
	}

}