/*
SDLEngine
Matt Hoyle
*/
#include "render_buffer.h"
#include "utils.h"
#include <glew.h>
#include "core/profiler.h"

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

	bool RenderBuffer::Create(size_t bufferSize, RenderBufferType type, RenderBufferModification modification)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(bufferSize > 0, "Buffer size must be >0");

		if (bufferSize > 0)
		{
			auto bufferType = TranslateBufferType(type);

			glGenBuffers(1, &m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glGenBuffers");

			glBindBuffer(bufferType, m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glBindBuffer");

			// We initialise the buffer memory to the correct size, but we do NOT populate it
			auto usageType = TranslateModificationType(modification);
			glBufferData(bufferType, bufferSize, nullptr, usageType);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glBufferData");

			// Reset state
			glBindBuffer(bufferType, 0);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glBindBuffer");

			m_bufferSize = bufferSize;
			m_type = type;
		}

		return true;
	}

	void RenderBuffer::SetData(size_t offset, size_t size, void* srcData)
	{
		SDE_PROF_EVENT();
		auto bufferType = TranslateBufferType(m_type);

		SDE_ASSERT(offset < m_bufferSize);
		SDE_ASSERT((size + offset) <= m_bufferSize);
		SDE_ASSERT(srcData != nullptr);
		SDE_ASSERT(m_handle != 0);

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