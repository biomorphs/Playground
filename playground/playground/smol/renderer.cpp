#include "renderer.h"
#include "kernel/assert.h"
#include "kernel/log.h"
#include "core/profiler.h"
#include "core/string_hashing.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/device.h"
#include "render/mesh.h"
#include "render/material.h"
#include "mesh_instance.h"
#include "model_manager.h"
#include "shader_manager.h"
#include "model.h"
#include "material_helpers.h"
#include <algorithm>
#include <map>

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

	struct GlobalUniforms
	{
		glm::mat4 m_projectionMat;
		glm::mat4 m_viewMat;
		glm::vec4 m_cameraPosition;		// world-space
		LightInfo m_lights[c_maxLights];
		int m_lightCount;
		float m_hdrExposure;
	};

	std::map<std::string, TextureHandle> g_defaultTextures;
	ShaderHandle g_basicBlitShader;

	Renderer::Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, glm::ivec2 windowSize)
		: m_textures(ta)
		, m_models(mm)
		, m_shaders(sm)
		, m_windowSize(windowSize)
		, m_mainFramebuffer(windowSize)
		, m_shadowDepthBuffer(glm::ivec2(c_shadowMapSize, c_shadowMapSize))
	{
		g_defaultTextures["DiffuseTexture"] = m_textures->LoadTexture("white.bmp");
		g_defaultTextures["NormalsTexture"] = m_textures->LoadTexture("default_normalmap.png");
		g_defaultTextures["SpecularTexture="] = m_textures->LoadTexture("white.bmp");
		g_basicBlitShader = m_shaders->LoadShader("Basic Blit", "basic_blit.vs", "basic_blit.fs");
		{
			SDE_PROF_EVENT("Create Buffers");
			CreateInstanceList(m_opaqueInstances, c_maxInstances);
			CreateInstanceList(m_transparentInstances, c_maxInstances);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferType::UniformData, Render::RenderBufferModification::Dynamic, true);
		}
		{
			SDE_PROF_EVENT("Create render targets");
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

	void Renderer::CreateInstanceList(InstanceList& newlist, uint32_t maxInstances)
	{
		SDE_PROF_EVENT();
		newlist.m_instances.reserve(maxInstances);
		newlist.m_transforms.Create(maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
		newlist.m_colours.Create(maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
	}

	void Renderer::Reset() 
	{ 
		m_opaqueInstances.m_instances.clear();
		m_transparentInstances.m_instances.clear();
		m_lights.clear();
	}

	void Renderer::SetCamera(const Render::Camera& c)
	{ 
		m_camera = c;
	}

	bool IsMeshTransparent(const Render::Mesh& mesh, TextureManager& tm)
	{
		const uint32_t c_diffuseSampler = Core::StringHashing::GetHash("DiffuseTexture");
		const auto& samplers = mesh.GetMaterial().GetSamplers();
		const auto& diffuseSampler = samplers.find(c_diffuseSampler);
		if (diffuseSampler != samplers.end())
		{
			TextureHandle texHandle = { static_cast<uint16_t>(diffuseSampler->second.m_handle) };
			const auto theTexture = tm.GetTexture({ texHandle });
			if (theTexture && theTexture->GetComponentCount() == 4)
			{
				return true;
			}
		}
		return false;
	}

	void Renderer::SubmitInstance(glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader)
	{
		SDE_PROF_EVENT();

		bool isTransparent = colour.a != 1.0f;
		if (!isTransparent)
		{
			isTransparent = IsMeshTransparent(mesh, *m_textures);
		}
		InstanceList& instances = isTransparent ? m_transparentInstances : m_opaqueInstances;
		float distanceToCamera = glm::length(transform[3] - glm::vec4(m_camera.Position(),1.0f));
		instances.m_instances.push_back({ transform, colour, shader, &mesh, distanceToCamera });
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
				bool isTransparent = colour.a != 1.0f;
				if (!isTransparent)
				{
					isTransparent = IsMeshTransparent(*part.m_mesh, *m_textures);
				}
				InstanceList& instances = isTransparent ? m_transparentInstances : m_opaqueInstances;
				const glm::mat4 instanceTransform = transform * part.m_transform;
				float distanceToCamera = glm::length(instanceTransform[3] - glm::vec4(m_camera.Position(), 1.0f));
				instances.m_instances.push_back({ instanceTransform, colour, shader, part.m_mesh.get(), distanceToCamera });
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

	void Renderer::PopulateInstanceBuffers(InstanceList& list)
	{
		SDE_PROF_EVENT();

		//static to avoid constant allocations
		static std::vector<glm::mat4> instanceTransforms;
		instanceTransforms.reserve(list.m_instances.size());
		instanceTransforms.clear();
		static std::vector<glm::vec4> instanceColours;
		instanceColours.reserve(list.m_instances.size());
		instanceColours.clear();

		for (const auto& c : list.m_instances)
		{
			instanceTransforms.push_back(c.m_transform);
			instanceColours.push_back(c.m_colour);
		}

		// copy the instance buffers to gpu
		if (list.m_instances.size() > 0)
		{
			list.m_transforms.SetData(0, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
			list.m_colours.SetData(0, instanceColours.size() * sizeof(glm::vec4), instanceColours.data());
		}
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

	void Renderer::PrepareTransparentInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("Sort");
			std::sort(list.m_instances.begin(), list.m_instances.end(), [](const smol::MeshInstance& q1, const smol::MeshInstance& q2) -> bool {
				if (q1.m_distanceToCamera < q2.m_distanceToCamera)	// back to front
				{
					return false;
				}
				else if (q1.m_distanceToCamera > q2.m_distanceToCamera)
				{
					return true;
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

	void Renderer::PrepareOpaqueInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("Sort");
			std::sort(list.m_instances.begin(), list.m_instances.end(), [](const smol::MeshInstance& q1, const smol::MeshInstance& q2) -> bool {
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
				if (q1.m_distanceToCamera < q2.m_distanceToCamera)	// front to back
				{
					return true;
				}
				else if (q1.m_distanceToCamera > q2.m_distanceToCamera)
				{
					return false;
				}
				return false;
			});
		}
	}

	void Renderer::SubmitInstances(Render::Device& d, const InstanceList& list)
	{
		SDE_PROF_EVENT();
		auto firstInstance = list.m_instances.begin();
		const Render::ShaderProgram* lastShaderUsed = nullptr;	// avoid setting the same shader
		while (firstInstance != list.m_instances.end())
		{
			// Batch by shader and mesh
			auto lastMeshInstance = std::find_if(firstInstance, list.m_instances.end(), [firstInstance](const smol::MeshInstance& m) -> bool {
				return  m.m_mesh != firstInstance->m_mesh || m.m_shader.m_index != firstInstance->m_shader.m_index;
				});
			auto instanceCount = (uint32_t)(lastMeshInstance - firstInstance);
			const Render::Mesh* theMesh = firstInstance->m_mesh;
			const auto theShader = m_shaders->GetShader(firstInstance->m_shader);
			if (theShader != nullptr && theMesh != nullptr)
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
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, 0, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, sizeof(float) * 4, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, sizeof(float) * 8, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, sizeof(float) * 12, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_colours, instancingSlotIndex++, 4, 0);

				// apply mesh material uniforms and samplers
				smol::ApplyMaterial(d, *theShader, theMesh->GetMaterial(), *m_textures, g_defaultTextures);

				// draw the chunks
				for (const auto& chunk : theMesh->GetChunks())
				{
					uint32_t firstIndex = (uint32_t)(firstInstance - list.m_instances.begin());
					d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex);
					m_frameStats.m_drawCalls++;
					m_frameStats.m_totalVertices += chunk.m_vertexCount * instanceCount;
				}
			}
			firstInstance = lastMeshInstance;
		}
	}

	void Renderer::RenderAll(Render::Device& d)
	{
		SDE_PROF_EVENT();
		auto totalInstances = m_opaqueInstances.m_instances.size() + m_transparentInstances.m_instances.size();
		m_frameStats = { totalInstances,0,0,0,0 };
		{
			SDE_PROF_EVENT("Clear main framebuffer");
			// clear targets asap
			d.SetDepthState(true, true);	// make sure depth write is enabled before clearing!
			d.ClearFramebufferColourDepth(m_mainFramebuffer, m_clearColour, FLT_MAX);
		}
		// set global data
		UpdateGlobals();

		// prepare instance lists for passes
		PrepareOpaqueInstances(m_opaqueInstances);
		PrepareTransparentInstances(m_transparentInstances);

		// copy instance data to gpu
		PopulateInstanceBuffers(m_opaqueInstances);
		PopulateInstanceBuffers(m_transparentInstances);

		d.DrawToFramebuffer(m_mainFramebuffer);
		d.SetViewport(glm::ivec2(0,0), m_mainFramebuffer.Dimensions());

		// render opaques
		d.SetBackfaceCulling(true, true);	// backface culling, ccw order
		d.SetBlending(false);				// no blending for opaques
		d.SetScissorEnabled(false);			// (don't) scissor me timbers
		SubmitInstances(d, m_opaqueInstances);

		// render transparents
		d.SetDepthState(true, false);		// enable z-test, disable write
		d.SetBlending(true);
		SubmitInstances(d, m_transparentInstances);

		// blit main buffer to backbuffer
		d.SetDepthState(false, false);
		d.SetBlending(true);
		auto blitShader = m_shaders->GetShader(g_basicBlitShader);
		if (blitShader)
		{
			m_targetBlitter.TargetToBackbuffer(d, m_mainFramebuffer, *blitShader, m_windowSize);
		}
	}
}