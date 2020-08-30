#pragma once
#include "render/render_pass.h"
#include "render/render_buffer.h"
#include "render/frame_buffer.h"
#include "render/camera.h"
#include "math/glm_headers.h"
#include "mesh_instance.h"
#include "light.h"
#include <vector>
#include <memory>

namespace Render
{
	class Material;
}

namespace smol
{
	class TextureManager;
	class ModelManager;
	class ShaderManager;

	class Renderer : public Render::RenderPass
	{
	public:
		Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, glm::ivec2 windowSize);			
		virtual ~Renderer() = default;

		void Reset();
		void RenderAll(Render::Device&);
		void SetCamera(const Render::Camera& c);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const struct ModelHandle& model, const struct ShaderHandle& shader);
		void SetLight(glm::vec4 positionOrDir,glm::vec3 colour, float ambientStr, glm::vec3 attenuation);
		Render::FrameBuffer& GetMainFramebuffer() { return m_mainFramebuffer; }
		struct FrameStats {
			size_t m_instancesSubmitted;
			size_t m_shaderBinds;
			size_t m_vertexArrayBinds;
			size_t m_batchesDrawn;
			size_t m_drawCalls;
			size_t m_totalVertices;
		};
		const FrameStats& GetStats() const { return m_frameStats; }
		float& GetExposure() { return m_hdrExposure; }
	private:
		void ApplyMaterial(Render::Device& d, Render::ShaderProgram& shader, const Render::Material& material);
		void PrepareInstances();
		void UpdateGlobals();
		void PopulateInstanceBuffers();
		struct GlobalUniforms;
		Render::Camera m_camera;
		glm::ivec2 m_windowSize;
		std::vector<MeshInstance> m_instances;
		std::vector<Light> m_lights;
		ShaderManager* m_shaders;
		smol::TextureManager* m_textures;
		smol::ModelManager* m_models;
		Render::RenderBuffer m_instanceTransforms;
		Render::RenderBuffer m_instanceColours;
		Render::RenderBuffer m_globalsUniformBuffer;
		Render::FrameBuffer m_mainFramebuffer;
		Render::FrameBuffer m_shadowDepthBuffer;
		FrameStats m_frameStats;
		float m_hdrExposure = 1.0f;
	};
}
