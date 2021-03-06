#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include "render/texture.h"
#include "render/texture_source.h"
#include "kernel/mutex.h"
#include "kernel/atomics.h"

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

		TextureHandle LoadTexture(std::string path);
		Render::Texture* GetTexture(const TextureHandle& h);
		void ProcessLoadedTextures();

		bool ShowGui(DebugGui::DebugGuiSystem& gui);

		void ReloadAll();

	private:
		struct TextureDesc {
			std::unique_ptr<Render::Texture> m_texture;
			std::string m_path;
		};
		std::vector<TextureDesc> m_textures;

		struct LoadedTexture
		{
			std::unique_ptr<Render::Texture> m_texture;
			TextureHandle m_destination;
		};
		Kernel::Mutex m_loadedTexturesMutex;
		std::vector<LoadedTexture> m_loadedTextures;
		Kernel::AtomicInt32 m_inFlightTextures = 0;
		SDE::JobSystem* m_jobSystem = nullptr;
	};
}