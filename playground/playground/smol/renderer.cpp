#include "renderer.h"
#include "kernel/log.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/device.h"
#include "render/mesh.h"
#include "render/uniform_buffer.h"
#include "mesh_instance.h"
#include "model_manager.h"
#include "model.h"
#include <algorithm>

namespace smol
{
	struct GlobalUniforms
	{
		glm::mat4 m_projectionMat;
		glm::mat4 m_viewMat;
	};

	void Renderer::Reset() 
	{ 
		m_instances.clear(); 
	}

	void Renderer::SetCamera(const Render::Camera& c)
	{ 
		m_camera = c;
	}

	void Renderer::SubmitInstance(glm::mat4 transform, glm::vec4 colour, const struct ModelHandle& model)
	{
		const auto theModel = m_models->GetModel(model);
		if (theModel != nullptr)
		{
			for (const auto& part : theModel->Parts())
			{
				const uint64_t meshHash = reinterpret_cast<uintptr_t>(part.m_mesh) & 0x00000000ffffffff;
				const uint64_t textureHash = part.m_diffuse.m_index & 0x00000000ffffffff;
				const uint64_t sortKey = textureHash | (meshHash << 32);
				m_instances.push_back({ sortKey, transform * part.m_transform, colour, part.m_diffuse, part.m_mesh });
			}
		}
	}

	void Renderer::SetupShader(Render::Device& d)
	{
		m_shaders = std::make_unique<Render::ShaderProgram>();

		auto vertexShader = std::make_unique<Render::ShaderBinary>();
		std::string errorText;
		if (!vertexShader->CompileFromFile(Render::ShaderType::VertexShader, "cube.vs", errorText))
		{
			SDE_LOG("Vertex shader compilation failed - %s", errorText.c_str());
		}
		auto fragmentShader = std::make_unique<Render::ShaderBinary>();
		if (!fragmentShader->CompileFromFile(Render::ShaderType::FragmentShader, "cube.fs", errorText))
		{
			SDE_LOG("Fragment shader compilation failed - %s", errorText.c_str());
		}

		if (!m_shaders->Create(*vertexShader, *fragmentShader, errorText))
		{
			SDE_LOG("Shader linkage failed - %s", errorText.c_str());
		}

		// Bind the globals UBO to index 0
		d.BindUniformBufferIndex(*m_shaders, "Globals", 0);
	}

	void Renderer::PopulateInstanceBuffers()
	{
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
		static bool s_firstFrame = true;
		if (s_firstFrame)
		{
			m_instanceTransforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
			m_instanceColours.Create(c_maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferType::UniformData, Render::RenderBufferModification::Static);

			SetupShader(d);
			s_firstFrame = false;
		}

		if (m_instances.size() == 0)
		{
			return;
		}

		// sort by generic sort key
		std::sort(m_instances.begin(), m_instances.end(), [](const smol::MeshInstance& q1, const smol::MeshInstance& q2) -> bool {
			return q1.m_sortKey < q2.m_sortKey;
			});

		PopulateInstanceBuffers();

		// global uniforms
		Render::UniformBuffer ub;
		auto windowSize = glm::vec2(m_windowSize.x, m_windowSize.y);

		// render state
		d.SetDepthState(true, true);		// enable z-test, enable write
		d.SetBackfaceCulling(true, true);	// backface culling, ccw order
		d.SetBlending(true);				// enable blending (we might want to do it manually instead)
		d.SetScissorEnabled(false);			// (don't) scissor me timbers
		d.BindShaderProgram(*m_shaders);

		GlobalUniforms globals;
		globals.m_projectionMat = glm::perspectiveFov(glm::radians(75.0f), windowSize.x, windowSize.y, 0.1f, 1000.0f);
		globals.m_viewMat = glm::lookAt(m_camera.Position(), m_camera.Target(), m_camera.Up());
		m_globalsUniformBuffer.SetData(0, sizeof(globals), &globals);

		d.SetUniforms(*m_shaders, m_globalsUniformBuffer, 0);	// do this for every shader change

		auto firstInstance = m_instances.begin();
		while (firstInstance != m_instances.end())
		{
			const Render::Mesh* theMesh =firstInstance->m_mesh;

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
				uint64_t texID = firstTexInstance->m_texture.m_index;
				auto lastTexInstance = std::find_if(firstTexInstance, lastMeshInstance, [texID](const smol::MeshInstance& m) -> bool {
					return m.m_texture.m_index != texID;
					});

				// firstTexInstance -> lastTexInstance instances to draw
				smol::TextureHandle texture = texID != (uint64_t)-1 ? firstTexInstance->m_texture : smol::TextureHandle{ 0 };
				ub.SetSampler("MyTexture", m_textures->GetTexture(texture)->GetHandle());
				
				d.SetUniforms(*m_shaders, ub);
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