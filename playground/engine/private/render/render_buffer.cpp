/*
SDLEngine
Matt Hoyle
*/
#include "render_buffer.h"
#include "utils.h"
#include <glew.h>
#include "core/profiler.h"
#include <cstring>

namespace Render
{
	RenderBuffer::RenderBuffer()
		: m_bufferSize(0)
		, m_handle(0)
	{
	}

	RenderBuffer::~RenderBuffer()
	{
		Destroy();
	}

	inline uint32_t RenderBuffer::TranslateBufferType(RenderBufferType type) const
	{
		switch (type)
		{
		case RenderBufferType::VertexData:
			return GL_ARRAY_BUFFER;
		case RenderBufferType::IndexData:
			return GL_ELEMENT_ARRAY_BUFFER;
		case RenderBufferType::UniformData:
			return GL_UNIFORM_BUFFER;
		default:
			return -1;
		}
	}

	uint32_t RenderBuffer::TranslateModificationType(RenderBufferModification type) const
	{
		switch (type)
		{
		case RenderBufferModification::Static:
			return GL_STATIC_DRAW;
		case RenderBufferModification::Dynamic:
			return GL_DYNAMIC_DRAW;
		default:
			return -1;
		}
	}

	uint32_t RenderBuffer::TranslateStorageType(RenderBufferModification type) const
	{
		switch (type)
		{
		case RenderBufferModification::Static:
			return 0;
		case RenderBufferModification::Dynamic:
			return GL_DYNAMIC_STORAGE_BIT;
		default:
			return -1;
		}
	}

	bool RenderBuffer::Create(void* sourceData, size_t bufferSize, RenderBufferType type, RenderBufferModification modification)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(bufferSize > 0, "Buffer size must be >0");

		if (bufferSize > 0)
		{
			auto bufferType = TranslateBufferType(type);

			{
				SDE_PROF_EVENT("glCreateBuffers");
				glCreateBuffers(1, &m_handle);	// create + init an unbound buffer object
				SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateBuffers");
			}

			{
				SDE_PROF_EVENT("glNamedBufferStorage");
				SDE_RENDER_ASSERT(!(modification == RenderBufferModification::Static && sourceData == nullptr), "Buffer must be dynamic");
				auto storageType = TranslateStorageType(modification);
				glNamedBufferStorage(m_handle, bufferSize, sourceData, storageType);
				SDE_RENDER_PROCESS_GL_ERRORS_RET("glNamedBufferStorage");
			}

			m_bufferSize = bufferSize;
			m_type = type;
		}

		return true;
	}

	bool RenderBuffer::Create(size_t bufferSize, RenderBufferType type, RenderBufferModification modification)
	{
		return Create(nullptr, bufferSize, type, modification);
	}

	void RenderBuffer::SetData(size_t offset, size_t size, void* srcData)
	{
		SDE_PROF_EVENT();
		SDE_ASSERT(offset < m_bufferSize);
		SDE_ASSERT((size + offset) <= m_bufferSize);
		SDE_ASSERT(srcData != nullptr);
		SDE_ASSERT(m_handle != 0);
		
		auto bufferType = TranslateBufferType(m_type);
		{
			SDE_PROF_EVENT("glNamedBufferSubData");
			glNamedBufferSubData(m_handle, offset, size, srcData);
			SDE_RENDER_PROCESS_GL_ERRORS("glNamedBufferSubData");
		}
	}

	bool RenderBuffer::Destroy()
	{
		SDE_PROF_EVENT();
		if (m_handle != 0)
		{
			glDeleteBuffers(1, &m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS("glDeleteBuffers");
			m_handle = 0;
			m_bufferSize = 0;
		}

		return true;
	}
}