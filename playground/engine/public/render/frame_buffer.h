#pragma once
#include "math/glm_headers.h"
#include <vector>
#include <memory>

namespace Render
{
	class Texture;
	class FrameBuffer
	{
	public:
		FrameBuffer(glm::vec2 size);
		~FrameBuffer();

		bool AddColourAttachment();
		bool AddDepthStencil();
		bool Create();
		void Destroy();

		uint32_t GetHandle() const { return m_fboHandle; }
		int GetColourAttachmentCount() const { return (int)m_colourAttachments.size(); }
		inline Texture& GetColourAttachment(int index) { return *m_colourAttachments[index]; }
		const std::unique_ptr<Texture>& GetDepthStencil() const { return m_depthStencil; }

	private:
		std::vector<std::unique_ptr<Texture>> m_colourAttachments;
		std::unique_ptr<Texture> m_depthStencil;
		glm::ivec2 m_dimensions;
		uint32_t m_fboHandle = 0;
	};
}