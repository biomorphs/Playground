/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include <memory>
#include "texture_source.h"

namespace Render
{
	class DDSLoader
	{
	public:
		DDSLoader();
		~DDSLoader();

		std::unique_ptr<TextureSource> LoadFile(const char* path);

	private:
		bool IsDDSFormat() const;
		TextureSource::Format ExtractFormat(uint32_t formatFourcc) const;
		bool ExtractHeaderData();
		bool ExtractMips(uint32_t mipCount);

		TextureSource::Format m_format;
		uint32_t m_width;
		uint32_t m_height;
		std::vector<TextureSource::MipDesc> m_mipDescriptors;
		std::vector<uint8_t> m_rawBuffer;
	};
}