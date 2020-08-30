#include "renderer.h"
#include "kernel/assert.h"
#include "kernel/log.h"
#include "core/profiler.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/device.h"
#include "render/mesh.h"
#include "render/material.h"
#include "mesh_instance.h"
#include "model_manager.h"
#include "shader_manager.h"
#include "model.h"
#include <algorithm>

namespace smol
{
	const uint64_t c_maxInstances = 1024 * 128;
	const uint64_t c_maxLights = 64;
	const int c_shadowMapSize = 1024;

	struct LightInfo
	{
		glm::vec4 m_colourAndAmbient;	// rgb=colour, a=ambient multiplier
		glm::vec4 m_position;
		glm::vec3 m_attenuation;		// const, linear, quad
	};

	struct Renderer::GlobalUniforms
	{
		glm::mat4 m_projectionMat;
		glm::mat4 m_viewMat;
		glm::vec4 m_cameraPosition;		// world-space
		LightInfo m_lights[c_maxLights];
		int m_lightCount;
		float m_hdrExposure;
	};

	Renderer::Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, glm::ivec2 windowSize)
		: m_textures(ta)
		, m_models(mm)
		, m_shaders(sm)
		, m_windowSize(windowSize)
		, m_mainFramebuffer(windowSize)
		, m_shadowDepthBuffer(glm::ivec2(c_shadowMapSize, c_shadowMapSize))
	{
		m_whiteTexture = m_textures->LoadTexture("white.bmp");
		m_defaultNormalmap = m_textures->LoadTexture("default_normalmap.png");

		{
			SDE_PROF_EVENT("Create Instance Buffers");
			m_instanceTransforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
			m_instanceColours.Create(c_maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferType::UniformData, Render::RenderBufferModification::Dynamic, true);
		}
		{
			SDE_PROF_EVENT("Create framebuffers");
			m_mainFramebuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F16);
			m_mainFramebuffer.AddDepthStencil();
			if (!m_mainFramebuffer.Create())
			{
				SDE_LOG("Failed to create framebuffer!");
			}
			m_shadowDepthBuffer.AddDepth();
			if (!m_shadowDepthBuffer.Create())
			{
				SDE_LOG("Failed to create shadow depth buffer");
			}
		}
	}

	void Renderer::Reset() 
	{ 
		m_instances.clear(); 
		m_lights.clear();
	}

	void Renderer::SetCamera(const Render::Camera& c)
	{ 
		m_camera = c;
	}

	void Renderer::SubmitInstance(glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader)
	{
		SDE_PROF_EVENT();
		m_instances.push_back({ transform, 0.0f, colour, TextureHandle(), TextureHandle(), TextureHandle(), shader, &mesh });
	}

	void Renderer::SubmitInstance(glm::mat4 transform, glm::vec4 colour, const struct ModelHandle& model, const struct ShaderHandle& shader)
	{
		SDE_PROF_EVENT();

		const auto theModel = m_models->GetModel(model);
		const auto theShader = m_shaders->GetShader(shader);
		if (theModel != nullptr && theShader != nullptr)
		{
			uint16_t meshPartIndex = 0;
			for (const auto& part : theModel->Parts())
			{
				m_instances.push_back({ transform * part.m_transform, 0.0f, colour, part.m_diffuse, part.m_normalMap, part.m_specularMap, shader, part.m_mesh.get() });
			}
		}
	}

	void Renderer::SetLight(glm::vec4 positionOrDir, glm::vec3 colour, float ambientStr, glm::vec3 attenuation)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = positionOrDir;
		newLight.m_attenuation = attenuation;
		m_lights.push_back(newLight);
	}

	void Renderer::PopulateInstanceBuffers()
	{
		SDE_PROF_EVENT();

		//static to avoid constant allocations
		static std::vector<glm::mat4> instanceTransforms;
		instanceTransforms.reserve(c_maxInstances);
		instanceTransforms.clear();
		static std::vector<glm::vec4> instanceColours;
		instanceColours.reserve(c_maxInstances);
		instanceColours.clear();
		for (const auto& c : m_instances)
		{
			instanceTransforms.push_back(c.m_transform);
			instanceColours.push_back(c.m_colour);
		}

		// copy the instance buffers to gpu
		m_instanceTransforms.SetData(0, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
		m_instanceColours.SetData(0, instanceColours.size() * sizeof(glm::vec4), instanceColours.data());
	}

	void Renderer::UpdateGlobals()
	{
		SDE_PROF_EVENT();
		GlobalUniforms globals;
		globals.m_projectionMat = glm::perspectiveFov(glm::radians(70.0f), (float)m_windowSize.x, (float)m_windowSize.y, 0.1f, 1000.0f);
		globals.m_viewMat = glm::lookAt(m_camera.Position(), m_camera.Target(), m_camera.Up());
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			globals.m_lights[l].m_colourAndAmbient = m_lights[l].m_colour;
			globals.m_lights[l].m_position = m_lights[l].m_position;
			globals.m_lights[l].m_attenuation = m_lights[l].m_attenuation;
		}
		globals.m_lightCount = static_cast<int>(std::min(m_lights.size(), c_maxLights));
		globals.m_cameraPosition = glm::vec4(m_camera.Position(), 0.0);
		globals.m_hdrExposure = m_hdrExposure;
		m_globalsUniformBuffer.SetData(0, sizeof(globals), &globals);
	}

	void Renderer::PrepareInstances()
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("DistanceToCamera");
			for (auto& c : m_instances)
			{
				float distance = glm::length(m_camera.Position() - glm::vec3(c.m_transform[3]));
				c.m_distanceToCamera = distance;
			}
		}
		{
			SDE_PROF_EVENT("Sort");
			std::sort(m_instances.begin(), m_instances.end(), [](const smol::MeshInstance& q1, const smol::MeshInstance& q2) -> bool {
				if (q1.m_distanceToCamera < q2.m_distanceToCamera)	// front to back
				{
					return true;		
				}
				else if (q1.m_distanceToCamera > q2.m_distanceToCamera)
				{
					return false;
				}
				if (q1.m_shader.m_index < q2.m_shader.m_index)	// shader
				{
					return true;
				}
				else if (q1.m_shader.m_index > q2.m_shader.m_index)
				{
					return false;
				}
				auto q1Mesh = reinterpret_cast<uintptr_t>(q1.m_mesh);	// mesh
				auto q2Mesh = reinterpret_cast<uintptr_t>(q2.m_mesh);
				if (q1Mesh < q2Mesh)
				{
					return true;
				}
				else if (q1Mesh > q2Mesh)
				{
					return false;
				}
				return false;
			});
		}
	}

	void Renderer::RenderAll(Render::Device& d)
	{
		SDE_PROF_EVENT();
		m_frameStats = { m_instances.size(),0,0,0,0 };
		if (m_instances.size() == 0)
		{
			return;
		}
		UpdateGlobals();
		const auto c_defaultTexture = m_textures->GetTexture(m_whiteTexture);
		const auto c_defaultNormalmap = m_textures->GetTexture(m_defaultNormalmap);
		if (c_defaultTexture == nullptr || c_defaultNormalmap == nullptr)
		{
			return;
		}
		PrepareInstances();
		PopulateInstanceBuffers();

		// render to main target
		d.DrawToFramebuffer(m_mainFramebuffer);
		d.ClearFramebufferColourDepth(m_mainFramebuffer, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), FLT_MAX);
		d.SetDepthState(true, true);		// enable z-test, enable write
		d.SetBackfaceCulling(true, true);	// backface culling, ccw order
		d.SetBlending(true);				// enable blending (we might want to do it manually instead)
		d.SetScissorEnabled(false);			// (don't) scissor me timbers
		auto firstInstance = m_instances.begin();
		const Render::ShaderProgram* lastShaderUsed = nullptr;
		while (firstInstance != m_instances.end())
		{
			const Render::Mesh* theMesh = firstInstance->m_mesh;
			// Batch by shader and mesh
			auto lastMeshInstance = std::find_if(firstInstance, m_instances.end(), [firstInstance](const smol::MeshInstance& m) -> bool {
				return  m.m_mesh != firstInstance->m_mesh || m.m_shader.m_index != firstInstance->m_shader.m_index;
				});
			auto instanceCount = (uint32_t)(lastMeshInstance - firstInstance);
			const auto theShader = m_shaders->GetShader(firstInstance->m_shader);
			if (theShader != nullptr)
			{
				m_frameStats.m_batchesDrawn++;

				// bind shader + globals UBO
				if (theShader != lastShaderUsed)
				{
					m_frameStats.m_shaderBinds++;
					d.BindShaderProgram(*theShader);
					d.BindUniformBufferIndex(*theShader, "Globals", 0);
					d.SetUniforms(*theShader, m_globalsUniformBuffer, 0);
					lastShaderUsed = theShader;
				}

				// bind vertex array + instancing streams immediately after mesh vertex streams
				m_frameStats.m_vertexArrayBinds++;
				int instancingSlotIndex = theMesh->GetVertexArray().GetStreamCount();
				d.BindVertexArray(theMesh->GetVertexArray());
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, 0, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 4, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 8, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 12, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceColours, instancingSlotIndex++, 4, 0);

				const auto& uniforms = theMesh->GetMaterial().GetUniforms();
				uniforms.Apply(d, *theShader);

				// todo - refactor
				Render::UniformBuffer ub;
				auto diffusePtr = m_textures->GetTexture(firstInstance->m_texture);
				ub.SetSampler("DiffuseTexture", diffusePtr ? diffusePtr->GetHandle() : c_defaultTexture->GetHandle());
				auto normalPtr = m_textures->GetTexture(firstInstance->m_normalTexture);
				ub.SetSampler("NormalsTexture", normalPtr ? normalPtr->GetHandle() : c_defaultNormalmap->GetHandle());
				auto specPtr = m_textures->GetTexture(firstInstance->m_specularTexture);
				ub.SetSampler("SpecularTexture", specPtr ? specPtr->GetHandle() : c_defaultTexture->GetHandle());
				ub.Apply(d, *theShader);

				// draw the chunks
				for (const auto& chunk : theMesh->GetChunks())
				{
					uint32_t firstIndex = (uint32_t)(firstInstance - m_instances.begin());
					d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex);
					m_frameStats.m_drawCalls++;
					m_frameStats.m_totalVertices += chunk.m_vertexCount * instanceCount;
				}
			}
			firstInstance = lastMeshInstance;
		}
		d.DrawToBackbuffer();
	}
}