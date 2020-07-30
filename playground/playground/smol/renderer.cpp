#include "renderer.h"
#include "kernel/assert.h"
#include "kernel/log.h"
#include "core/profiler.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/device.h"
#include "render/mesh.h"
#include "render/uniform_buffer.h"
#include "mesh_instance.h"
#include "model_manager.h"
#include "shader_manager.h"
#include "model.h"
#include <algorithm>

namespace smol
{
	const uint64_t c_maxInstances = 1024 * 128;
	const uint64_t c_maxLights = 64;

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
	};

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

		// Shader - bits 64 - 48
		const uint64_t shaderHash = static_cast<uint64_t>(shader.m_index) << 48;

		// Then mesh - bits 48-24
		const uint64_t meshHash = (reinterpret_cast<uintptr_t>(&mesh) & 0x0000000000ffffff) << 24;

		// No textures

		const uint64_t sortKey = meshHash | shaderHash;
		m_instances.push_back({ sortKey, transform, colour, TextureHandle(), TextureHandle(), TextureHandle(), shader, &mesh });
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
				// Shader - bits 64 - 48
				const uint64_t shaderHash = static_cast<uint64_t>(shader.m_index) << 48;

				// Then mesh/model - bits 48-32 for model, 32-24 bits per mesh
				const uint64_t modelHash = static_cast<uint64_t>(model.m_index) << 32;
				const uint64_t meshHash = static_cast<uint64_t>(meshPartIndex) << 24;

				// bits 24 - 8 for diffuse
				const uint64_t textureHash = static_cast<uint64_t>(part.m_diffuse.m_index) << 8;

				// 1 byte spare

				const uint64_t sortKey = textureHash | meshHash | modelHash | shaderHash;
				m_instances.push_back({ sortKey, transform * part.m_transform, colour, part.m_diffuse, part.m_normalMap, part.m_specularMap, shader, part.m_mesh });
			}
			SDE_ASSERT(meshPartIndex <= 255, "Too many parts in mesh!");
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

	void Renderer::RenderAll(Render::Device& d)
	{
		SDE_PROF_EVENT();

		static bool s_firstFrame = true;
		if (s_firstFrame)
		{
			SDE_PROF_EVENT("Create Instance Buffers");
			m_instanceTransforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
			m_instanceColours.Create(c_maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferType::UniformData, Render::RenderBufferModification::Static);
			m_whiteTexture = m_textures->LoadTexture("white.bmp");
			m_defaultNormalmap = m_textures->LoadTexture("default_normalmap.png");
			s_firstFrame = false;
		}

		if (m_instances.size() == 0)
		{
			return;
		}

		{
			SDE_PROF_EVENT("SortInstances");
			std::sort(m_instances.begin(), m_instances.end(), [](const smol::MeshInstance& q1, const smol::MeshInstance& q2) -> bool {
				return q1.m_sortKey < q2.m_sortKey;
				});
		}

		PopulateInstanceBuffers();
		auto windowSize = glm::vec2(m_windowSize.x, m_windowSize.y);

		// render state
		d.SetDepthState(true, true);		// enable z-test, enable write
		d.SetBackfaceCulling(true, true);	// backface culling, ccw order
		d.SetBlending(true);				// enable blending (we might want to do it manually instead)
		d.SetScissorEnabled(false);			// (don't) scissor me timbers

		{
			SDE_PROF_EVENT("Update Globals UBO");
			GlobalUniforms globals;
			globals.m_projectionMat = glm::perspectiveFov(glm::radians(70.0f), windowSize.x, windowSize.y, 0.1f, 1000.0f);
			globals.m_viewMat = glm::lookAt(m_camera.Position(), m_camera.Target(), m_camera.Up());
			for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
			{
				globals.m_lights[l].m_colourAndAmbient = m_lights[l].m_colour;
				globals.m_lights[l].m_position = m_lights[l].m_position;
				globals.m_lights[l].m_attenuation = m_lights[l].m_attenuation;
			}
			globals.m_lightCount = static_cast<int>(std::min(m_lights.size(), c_maxLights));
			globals.m_cameraPosition = glm::vec4(m_camera.Position(), 0.0);
			m_globalsUniformBuffer.SetData(0, sizeof(globals), &globals);
		}
		const auto c_defaultTexture = m_textures->GetTexture(m_whiteTexture);
		const auto c_defaultNormalmap = m_textures->GetTexture(m_defaultNormalmap);
		if (c_defaultTexture == nullptr || c_defaultNormalmap == nullptr)
		{
			return;
		}

		Render::UniformBuffer ub;	// hack for setting samplers
		auto firstInstance = m_instances.begin();
		while (firstInstance != m_instances.end())
		{
			const Render::Mesh* theMesh = firstInstance->m_mesh;
			const auto theShader = m_shaders->GetShader(firstInstance->m_shader);

			// bind shader + globals UBO
			d.BindShaderProgram(*theShader);
			d.BindUniformBufferIndex(*theShader, "Globals", 0);
			d.SetUniforms(*theShader, m_globalsUniformBuffer, 0);

			// bind vertex array
			d.BindVertexArray(theMesh->GetVertexArray());

			// bind instancing streams immediately after mesh vertex streams
			int instancingSlotIndex = theMesh->GetVertexArray().GetStreamCount();
			// matrices must be set over multiple stream as part of vao
			d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, 0, 4);
			d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 4, 4);
			d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 8, 4);
			d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceTransforms, instancingSlotIndex++, 4, sizeof(float) * 12, 4);
			d.BindInstanceBuffer(theMesh->GetVertexArray(), m_instanceColours, instancingSlotIndex++, 4, 0);

			// find the last matching mesh
			auto lastMeshInstance = std::find_if(firstInstance, m_instances.end(), [theMesh](const smol::MeshInstance& m) -> bool {
				return m.m_mesh != theMesh;
				});

			// 1 draw call per texture
			auto firstTexInstance = firstInstance;
			while (firstTexInstance != lastMeshInstance)
			{
				// firstTexInstance -> lastTexInstance instances to draw
				uint64_t texID = firstTexInstance->m_texture.m_index;
				auto lastTexInstance = std::find_if(firstTexInstance, lastMeshInstance, [texID](const smol::MeshInstance& m) -> bool {
					return m.m_texture.m_index != texID;
					});
				
				// textures
				auto diffusePtr = m_textures->GetTexture(firstTexInstance->m_texture);
				ub.SetSampler("DiffuseTexture", diffusePtr ? diffusePtr->GetHandle() : c_defaultTexture->GetHandle());
				
				auto normalPtr = m_textures->GetTexture(firstTexInstance->m_normalTexture);
				ub.SetSampler("NormalsTexture", normalPtr ? normalPtr->GetHandle() : c_defaultNormalmap->GetHandle());

				auto specPtr = m_textures->GetTexture(firstTexInstance->m_specularTexture);
				ub.SetSampler("SpecularTexture", specPtr ? specPtr->GetHandle() : c_defaultTexture->GetHandle());

				d.SetUniforms(*theShader, ub);

				for (const auto& chunk : theMesh->GetChunks())
				{
					auto instanceCount = (uint32_t)(lastTexInstance - firstTexInstance);
					uint32_t firstIndex = (uint32_t)(firstTexInstance - m_instances.begin());
					d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex);
				}
				firstTexInstance = lastTexInstance;
			}
			firstInstance = lastMeshInstance;
		}
	}
}