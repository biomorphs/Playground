#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "render/texture.h"

namespace smol
{
	struct TextureHandle
	{
		uint64_t m_index;
		static TextureHandle Invalid() { return { (uint64_t)-1 }; };
	};

	class TextureManager
	{
	public:
		TextureManager() = default;
		TextureManager(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = delete;
		~TextureManager();

		TextureHandle LoadTexture(const char* path);
		Render::Texture* GetTexture(const TextureHandle& h);
	private:
		struct TextureDesc {
			Render::Texture m_texture;
			std::string m_path;
		};
		std::vector<TextureDesc> m_textures;
	};
}