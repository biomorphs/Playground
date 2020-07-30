#pragma once
#include "render/render_pass.h"
#include "render/render_buffer.h"
#include "render/camera.h"
#include "math/glm_headers.h"
#include "mesh_instance.h"
#include "light.h"
#include <vector>
#include <memory>

namespace smol
{
	class TextureManager;
	class ModelManager;
	class ShaderManager;

	class Renderer : public Render::RenderPass
	{
	public:
		Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, glm::vec2 windowSize)
			: m_textures(ta)
			, m_models(mm)
			, m_shaders(sm)
			, m_windowSize(windowSize)
		{
		}
		virtual ~Renderer() = default;

		void Reset();
		void RenderAll(Render::Device&);
		void SetCamera(const Render::Camera& c);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const struct ModelHandle& model, const struct ShaderHandle& shader);
		void SetLight(glm::vec4 positionOrDir,glm::vec3 colour, float ambientStr, glm::vec3 attenuation);

	private:
		void PopulateInstanceBuffers();

		Render::Camera m_camera;
		glm::vec2 m_windowSize;
		std::vector<MeshInstance> m_instances;
		std::vector<Light> m_lights;
		smol::TextureHandle m_whiteTexture;
		smol::TextureHandle m_defaultNormalmap;
		smol::Light m_light;
		ShaderManager* m_shaders;
		smol::TextureManager* m_textures;
		smol::ModelManager* m_models;
		Render::RenderBuffer m_instanceTransforms;
		Render::RenderBuffer m_instanceColours;
		Render::RenderBuffer m_globalsUniformBuffer;
	};
}
