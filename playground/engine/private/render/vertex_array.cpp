/*
SDLEngine
Matt Hoyle
*/
#include "vertex_array.h"
#include "utils.h"
#include "render_buffer.h"
#include "core/profiler.h"

namespace Render
{
	// this must match the enum VertexDataType
	constexpr uint32_t VertexDataTypeSizes[] = {
		sizeof(float)
	};

	VertexArray::VertexArray()
		: m_handle(0)
	{
	}

	VertexArray::~VertexArray()
	{
		Destroy();
	}

	void VertexArray::AddBuffer(uint8_t attribIndex, const RenderBuffer* srcBuffer, VertexDataType srcType, uint8_t components, uint32_t offset, uint32_t stride)
	{
		SDE_ASSERT(srcBuffer != nullptr);
		SDE_ASSERT(components <= 4);

		// blindly add them, no real error checking, since (hopefully) Create will deal with it
		VertexBufferDescriptor newDesc;
		newDesc.m_srcBuffer = srcBuffer;
		newDesc.m_dataType = srcType;
		newDesc.m_componentCount = components;
		newDesc.m_offset = offset;
		if (stride == 0)
		{
			newDesc.m_stride = VertexDataTypeSizes[(int)srcType] * components;
		}
		else
		{
			newDesc.m_stride = stride;
		}
		newDesc.m_attribIndex = attribIndex;

		m_descriptors.push_back(newDesc);
	}

	inline uint32_t VertexArray::TranslateDataType(VertexDataType type) const
	{
		switch (type)
		{
		case VertexDataType::Float:
			return GL_FLOAT;
		default:
			return -1;
		}
	}

	bool VertexArray::Create()
	{
		SDE_PROF_EVENT();

		SDE_ASSERT(m_handle == 0);

		glCreateVertexArrays(1, &m_handle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateVertexArrays");

		for (auto it = m_descriptors.begin(); it != m_descriptors.end(); ++it)
		{
			// bind the VBs
			glVertexArrayVertexBuffer(m_handle, it->m_attribIndex, it->m_srcBuffer->GetHandle(), it->m_offset, it->m_stride);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glVertexArrayVertexBuffer");

			uint32_t glDataType = TranslateDataType(it->m_dataType);
			SDE_ASSERT(glDataType != -1);

			// set the array format
			glVertexArrayAttribFormat(m_handle, it->m_attribIndex, it->m_componentCount, glDataType, false, (GLuint)it->m_offset);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glVertexArrayAttribFormat");

			// enable the stream
			glEnableVertexArrayAttrib(m_handle, it->m_attribIndex);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glEnableVertexArrayAttrib");
		}

		return true;
	}

	void VertexArray::Destroy()
	{
		SDE_PROF_EVENT();

		// Note we do not destroy the buffers, only the VAO
		if (m_handle != 0)
		{
			glDeleteVertexArrays(1, &m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS("glDeleteVertexArrays");
		}		

		m_handle = 0;
	}
}