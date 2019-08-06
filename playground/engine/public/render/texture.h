/*
SDLEngine
Matt Hoyle
*/
#pragma once
#include "kernel/base_types.h"
#include <vector>

namespace Render
{
	class TextureSource;
	class Texture
	{
	public:
		Texture();
		Texture(const Texture&) = delete;
		Texture(Texture&& other) {
			m_isArray = other.m_isArray;
			m_handle = other.m_handle;
			other.m_handle = -1;
		}
		~Texture();

		bool Update(const std::vector<TextureSource>& src);
		bool Create(const std::vector<TextureSource>& src);
		void Destroy();

		inline uint32_t GetHandle() const { return m_handle; }
		inline bool IsArray() const { return m_isArray; }

	private:
		bool CreateSimpleUncompressedTexture(const TextureSource& src);
		bool CreateSimpleCompressedTexture(const TextureSource& src);
		bool CreateArrayCompressedTexture(const std::vector<TextureSource>& src);
		bool ValidateSource(const std::vector<TextureSource>& src);

		bool m_isArray;
		uint32_t m_handle;
	};
}