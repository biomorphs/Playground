/*
SDLEngine
Matt Hoyle
*/
#include "texture.h"
#include "texture_source.h"
#include "utils.h"
#include "core/profiler.h"

namespace Render
{
	Texture::Texture()
		: m_handle(-1)
	{
	}

	Texture::~Texture()
	{
		Destroy();
	}

	uint32_t SourceFormatToGLStorageFormat(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::DXT1:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case TextureSource::Format::DXT3:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case TextureSource::Format::DXT5:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case TextureSource::Format::RGBA8:
			return GL_RGBA8;
		default:
			return -1;
		}
	}

	uint32_t SourceFormatToGLInternalType(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::RGBA8:
			return GL_UNSIGNED_BYTE;
		default:
			return -1;
		}
	}

	uint32_t SourceFormatToGLInternalFormat(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::RGBA8:
			return GL_RGBA;
		default:
			return -1;
		}
	}

	bool ShouldCreateCompressed(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::DXT1:
			return true;
		case TextureSource::Format::DXT3:
			return true;
		case TextureSource::Format::DXT5:
			return true;
		default:
			return false;
		}
	}

	bool Texture::CreateSimpleUncompressedTexture(const TextureSource& src)
	{
		SDE_PROF_EVENT();

		glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateTextures");

		const uint32_t mipCount = src.MipCount();
		const bool shouldGenerateMips = mipCount == 1 && src.ShouldGenerateMips();
		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureParameteri");
		if (mipCount > 1 || shouldGenerateMips)
		{
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
		{
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureParameteri");

		uint32_t glStorageFormat = SourceFormatToGLStorageFormat(src.SourceFormat());
		uint32_t glInternalFormat = SourceFormatToGLInternalFormat(src.SourceFormat());
		uint32_t glInternalType = SourceFormatToGLInternalType(src.SourceFormat());
		SDE_RENDER_ASSERT(glStorageFormat != -1);
		SDE_RENDER_ASSERT(glInternalFormat != -1);
		SDE_RENDER_ASSERT(glInternalType != -1);
		{
			SDE_PROF_EVENT("AllocateStorage");
			// This preallocates the entire mip-chain
			glTextureStorage2D(m_handle, mipCount, glStorageFormat, src.Width(), src.Height());
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureStorage2D");
		}
		{
			SDE_PROF_EVENT("CopyData");
			for (uint32_t m = 0; m < mipCount; ++m)
			{
				uint32_t w = 0, h = 0;
				size_t size = 0;
				const uint8_t* mipData = src.MipLevel(m, w, h, size);
				SDE_ASSERT(mipData);

				glTextureSubImage2D(m_handle, m, 0, 0, w, h, glInternalFormat, glInternalType, mipData);
				SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureSubImage2D");
			}
		}
		if (shouldGenerateMips)
		{
			SDE_PROF_EVENT("GenerateMips");
			glGenerateTextureMipmap(m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glGenerateTextureMipmap");
		}
		return m_handle != 0;
	}

	bool Texture::CreateSimpleCompressedTexture(const TextureSource& src)
	{
		SDE_PROF_EVENT();

		glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateTextures");

		const uint32_t mipCount = src.MipCount();
		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureParameteri");
		glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureParameteri");

		uint32_t glFormat = SourceFormatToGLStorageFormat(src.SourceFormat());
		SDE_RENDER_ASSERT(glFormat != -1);

		// This preallocates the entire mip-chain
		glTextureStorage2D(m_handle, mipCount, glFormat, src.Width(), src.Height());
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureStorage2D");

		for (uint32_t m = 0; m < mipCount; ++m)
		{
			uint32_t w = 0, h = 0;
			size_t size = 0;
			const uint8_t* mipData = src.MipLevel(m, w, h, size);
			SDE_ASSERT(mipData);

			glCompressedTextureSubImage2D(m_handle, m, 0, 0, w, h, glFormat, (GLsizei)size, mipData);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glCompressedTextureSubImage2D");
		}
		return true;
	}

	bool Texture::CreateArrayCompressedTexture(const std::vector<TextureSource>& src)
	{
		SDE_PROF_EVENT();

		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_handle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateTextures");

		uint32_t glFormat = SourceFormatToGLStorageFormat(src[0].SourceFormat());
		SDE_RENDER_ASSERT(glFormat != -1);

		const uint32_t mipCount = src[0].MipCount();
		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureParameteri");
		glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureParameteri");

		// This preallocates the entire mip-chain for all textures in the array
		glTextureStorage3D(m_handle, mipCount, glFormat, src[0].Width(), src[0].Height(), (GLsizei)src.size());
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureStorage3D");

		// Now, since opengl is retarded and requires that glCompressedTexSubImage3D is passed all data for a mip level,
		// we must manually pack all data for a particular mip into one buffer
		uint32_t mip0w, mip0h;
		size_t mip0size;
		src[0].MipLevel(0, mip0w, mip0h, mip0size);	// Use first image to get total size required
		std::vector<uint8_t> mipBuffer(mip0size * src.size());
		for (uint32_t m = 0; m < mipCount; ++m)
		{
			uint32_t w = 0, h = 0;
			size_t size = 0;
			uint8_t* dataPtr = mipBuffer.data();
			for (uint32_t d = 0; d < src.size(); ++d)
			{
				const uint8_t* mipData = src[d].MipLevel(m, w, h, size);
				SDE_ASSERT(mipData);
				memcpy(dataPtr, mipData, size);
				dataPtr += size;
			}
			auto bytesWritten = (size_t)(dataPtr - mipBuffer.data());
			SDE_ASSERT(bytesWritten <= mipBuffer.size());

			glCompressedTextureSubImage3D(m_handle, m, 0, 0, 0, w, h, (GLsizei)src.size(), glFormat, (GLsizei)(bytesWritten), mipBuffer.data());
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glCompressedTextureSubImage3D");
		}
		mipBuffer.clear();

		return true;
	}

	bool Texture::ValidateSource(const std::vector<TextureSource>& src)
	{
		if (src.size() == 0)
		{
			return false;
		}
		if (src[0].MipCount() == 0)
		{
			return false;
		}

		uint32_t mipCount = src[0].MipCount();
		uint32_t width = src[0].Width();
		uint32_t height = src[0].Height();
		TextureSource::Format format = src[0].SourceFormat();
		for (auto& it : src)
		{
			if (mipCount != it.MipCount())
			{
				return false;
			}
			if (width != it.Width())
			{
				return false;
			}
			if (height != it.Height())
			{
				return false;
			}
			if (format != it.SourceFormat())
			{
				return false;
			}
		}

		return true;
	}

	bool Texture::Update(const std::vector<TextureSource>& src)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle != -1);
		SDE_ASSERT(src.size() > 0);
		if (!ValidateSource(src))
		{
			SDE_LOGC("Source data not valid for this texture");
			return false;
		}

		// only support uncompressed flat textures for now, laziness inbound
		if (ShouldCreateCompressed(src[0].SourceFormat()) || src.size() != 1)
		{
			return false;
		}

		uint32_t glStorageFormat = SourceFormatToGLStorageFormat(src[0].SourceFormat());
		uint32_t glInternalFormat = SourceFormatToGLInternalFormat(src[0].SourceFormat());
		uint32_t glInternalType = SourceFormatToGLInternalType(src[0].SourceFormat());
		SDE_RENDER_ASSERT(glStorageFormat != -1);
		SDE_RENDER_ASSERT(glInternalFormat != -1);
		SDE_RENDER_ASSERT(glInternalType != -1);
		const uint32_t mipCount = src[0].MipCount();
		for (uint32_t m = 0; m < mipCount; ++m)
		{
			uint32_t w = 0, h = 0;
			size_t size = 0;
			const uint8_t* mipData = src[0].MipLevel(m, w, h, size);
			SDE_ASSERT(mipData);

			glTextureSubImage2D(m_handle, m, 0, 0, w, h, glInternalFormat, glInternalType, mipData);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glTextureSubImage2D");
		}

		return true;
	}

	bool Texture::Create(const TextureSource& src)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle == -1);
		m_isArray = false;

		if (ShouldCreateCompressed(src.SourceFormat()))
		{
			return CreateSimpleCompressedTexture(src);
		}
		else
		{
			return CreateSimpleUncompressedTexture(src);
		}

		return false;
	}

	bool Texture::Create(const std::vector<TextureSource>& src)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle == -1);
		SDE_ASSERT(src.size() > 0);
		if (!ValidateSource(src))
		{
			SDE_LOGC("Source data not valid for this texture");
			return false;
		}

		if (ShouldCreateCompressed(src[0].SourceFormat()))
		{
			if (src.size() == 1)
			{
				m_isArray = false;
				return CreateSimpleCompressedTexture(src[0]);
			}
			else
			{
				m_isArray = true;
				return CreateArrayCompressedTexture(src);
			}
		}
		else
		{
			if (src.size() == 1)
			{
				m_isArray = false;
				return CreateSimpleUncompressedTexture(src[0]);
			}
		}
		return false;
	}

	void Texture::Destroy()
	{
		SDE_PROF_EVENT();
		if (m_handle != -1)
		{
			glDeleteTextures(1, &m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS("glDeleteTextures");
			m_handle = -1;
		}
	}
}