#pragma once
#include "render/render_pass.h"
#include "render/shader_program.h"
#include "render/render_buffer.h"
#include "render/camera.h"
#include "math/glm_headers.h"
#include <vector>
#include <memory>

namespace smol
{
	class TextureManager;
	class MeshManager;
	struct MeshInstance;
	struct MeshHandle;
	struct TextureHandle;

	class Renderer : public Render::RenderPass
	{
	public:
		Renderer(TextureManager* ta, MeshManager* ma, glm::vec2 windowSize)
			: m_textures(ta)
			, m_meshes(ma)
			, m_windowSize(windowSize)
		{
		}
		virtual ~Renderer() = default;

		void Reset();
		void RenderAll(Render::Device&);
		void SetCamera(const Render::Camera& c);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, MeshHandle mesh, TextureHandle texture);

	private:
		Render::Camera m_camera;
		void SetupShader();
		void PopulateInstanceBuffers();

		const uint64_t c_maxInstances = 1024 * 128;
		glm::vec2 m_windowSize;
		std::vector<smol::MeshInstance> m_instances;
		std::unique_ptr<Render::ShaderProgram> m_shaders;	// todo- shader manager?
		smol::TextureManager* m_textures;
		smol::MeshManager* m_meshes;
		Render::RenderBuffer m_instanceTransforms;
		Render::RenderBuffer m_instanceColours;
		Render::RenderBuffer m_globalsUniformBuffer;
	};
}
