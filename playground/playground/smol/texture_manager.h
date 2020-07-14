#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include "render/texture.h"
#include "render/texture_source.h"
#include "kernel/mutex.h"

namespace SDE
{
	class JobSystem;
}

namespace smol
{
	struct TextureHandle
	{
		uint16_t m_index = -1;
		static TextureHandle Invalid() { return { (uint16_t)-1 }; };
	};

	class TextureManager
	{
	public:
		TextureManager(SDE::JobSystem* js);
		TextureManager(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = delete;
		~TextureManager() = default;

		TextureHandle LoadTexture(const char* path);
		Render::Texture* GetTexture(const TextureHandle& h);
		void ProcessLoadedTextures();

	private:
		struct TextureDesc {
			std::unique_ptr<Render::Texture> m_texture;
			std::string m_path;
		};
		std::vector<TextureDesc> m_textures;

		struct LoadedTexture
		{
			Render::TextureSource m_texture;
			TextureHandle m_destination;
		};
		Kernel::Mutex m_loadedTexturesMutex;
		std::vector<LoadedTexture> m_loadedTextures;

		SDE::JobSystem* m_jobSystem;
	};
}