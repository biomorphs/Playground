/*
SDLEngine
Matt Hoyle
*/

#include "kernel/assert.h"

namespace Render
{
	inline TextureSource::TextureSource(uint32_t w, uint32_t h, Format f, std::vector<MipDesc>& mips, std::vector<uint8_t>& data)
		: m_width(w)
		, m_height(h)
		, m_format(f)
		, m_mipDescriptors(mips)
		, m_rawBuffer(data)
	{
	}

	inline TextureSource::TextureSource(uint32_t w, uint32_t h, Format f)
		: m_width(w)
		, m_height(h)
		, m_format(f)
	{
	}

	inline TextureSource::TextureSource(uint32_t w, uint32_t h, Format f, std::vector<MipDesc>& mips, std::vector<uint32_t>& data)
		: m_width(w)
		, m_height(h)
		, m_format(f)
		, m_mipDescriptors(mips)
	{
		m_rawBuffer.insert(m_rawBuffer.begin(), (uint8_t*)data.data(), (uint8_t*)data.data() + (data.size() * 4));
	}

	inline TextureSource::~TextureSource()
	{
	}

	inline const uint8_t* TextureSource::MipLevel(uint32_t mip, uint32_t& w, uint32_t& h, size_t& size) const
	{
		SDE_ASSERT(mip < m_mipDescriptors.size());
		w = m_mipDescriptors[mip].m_width;
		h = m_mipDescriptors[mip].m_height;
		size = m_mipDescriptors[mip].m_size;
		return m_rawBuffer.data() + m_mipDescriptors[mip].m_offset;
	}
}