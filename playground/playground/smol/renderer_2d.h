#pragma once
#include "render/render_pass.h"
#include "render/shader_program.h"
#include "render/render_buffer.h"
#include "math/glm_headers.h"
#include "texture_manager.h"
#include <vector>
#include <memory>

namespace Render
{
	class Mesh;
	class ShaderProgram;
}

namespace smol
{
	class TextureManager;

	class Renderer2D : public Render::RenderPass
	{
	public:
		Renderer2D(TextureManager* ta, glm::vec2 windowSize)
			: m_textures(ta)
			, m_windowSize(windowSize)
		{
		}
		virtual ~Renderer2D() = default;

		void Reset() { m_quads.clear(); }
		void RenderAll(Render::Device&);
		void SubmitQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour, const smol::TextureHandle& th);
	private:
		void BuildQuadMesh();
		void SetupQuadMaterial();
		void PopulateInstanceBuffers();

		struct Quad
		{
			glm::vec2 m_position;
			glm::vec2 m_size;
			glm::vec4 m_colour;
			TextureHandle m_texture;
		};

		const uint64_t c_maxQuads = 1024 * 128;
		std::vector<Quad> m_quads;
		glm::vec2 m_windowSize;
		std::unique_ptr<Render::Mesh> m_quadMesh;
		std::unique_ptr<Render::ShaderProgram> m_quadShaders;
		smol::TextureManager* m_textures;
		Render::RenderBuffer m_quadInstanceTransforms;
		Render::RenderBuffer m_quadInstanceColours;
	};
}