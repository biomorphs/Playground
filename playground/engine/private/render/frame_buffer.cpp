#include "frame_buffer.h"
#include "utils.h"
#include "texture.h"
#include "texture_source.h"

namespace Render
{
	FrameBuffer::FrameBuffer(glm::vec2 size)
		: m_dimensions(size)
	{
	}

	FrameBuffer::~FrameBuffer()
	{
		Destroy();
	}

	bool FrameBuffer::AddColourAttachment()
	{
		TextureSource ts(m_dimensions.x, m_dimensions.y, Render::TextureSource::Format::RGBA8);
		auto newTexture = std::make_unique<Render::Texture>();
		if (newTexture->Create(ts))
		{
			m_colourAttachments.push_back(std::move(newTexture));
			return true;
		}
		else
		{
			return false;
		}
	}

	bool FrameBuffer::AddDepthStencil()
	{
		TextureSource ts(m_dimensions.x, m_dimensions.y, Render::TextureSource::Format::Depth24Stencil8);
		auto newTexture = std::make_unique<Render::Texture>();
		if (newTexture->Create(ts))
		{
			m_depthStencil = std::move(newTexture);
			return true;
		}
		return false;
	}

	bool FrameBuffer::Create()
	{
		glCreateFramebuffers(1, &m_fboHandle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateFramebuffers");

		for (int c = 0; c < m_colourAttachments.size(); ++c)
		{
			glNamedFramebufferTexture(m_fboHandle, GL_COLOR_ATTACHMENT0 + c, m_colourAttachments[c]->GetHandle(), 0);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glNamedFramebufferTexture");
		}

		if (m_depthStencil != nullptr)
		{
			glNamedFramebufferTexture(m_fboHandle, GL_DEPTH_ATTACHMENT, m_depthStencil->GetHandle(), 0);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glNamedFramebufferTexture");
		}

		bool readyToGo = glCheckNamedFramebufferStatus(m_fboHandle, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCheckNamedFramebufferStatus");
		return readyToGo;
	}

	void FrameBuffer::Destroy()
	{
		if (m_fboHandle != 0)
		{
			glDeleteFramebuffers(1, &m_fboHandle);
			SDE_RENDER_PROCESS_GL_ERRORS("glDeleteFramebuffers");
		}

		m_colourAttachments.clear();
	}
}