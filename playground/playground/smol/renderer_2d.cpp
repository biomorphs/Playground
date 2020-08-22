#include "renderer_2d.h"
#include "kernel/log.h"
#include "render/mesh_builder.h"
#include "render/uniform_buffer.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/mesh.h"
#include <algorithm>
#include "core/profiler.h"

namespace smol
{
	void Renderer2D::SubmitQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour, const smol::TextureHandle& th)
	{
		m_quads.push_back({ pos, size, colour, th });
	}

	void Renderer2D::BuildQuadMesh()
	{
		m_quadMesh = std::make_unique<Render::Mesh>();
		Render::MeshBuilder builder;
		builder.AddVertexStream(2);		// position only
		builder.BeginChunk();
		builder.BeginTriangle();
		builder.SetStreamData(0, { 0,0 }, { 1,0 }, { 0,1 });	// ccw by default
		builder.EndTriangle();
		builder.BeginTriangle();
		builder.SetStreamData(0, { 0,1 }, { 1,0 }, { 1,1 });
		builder.EndTriangle();
		builder.EndChunk();
		builder.CreateMesh(*m_quadMesh);
		builder.CreateVertexArray(*m_quadMesh);

		// set up instance buffers
		m_quadInstanceTransforms.Create(c_maxQuads * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
		m_quadInstanceColours.Create(c_maxQuads * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic);
	}

	void Renderer2D::SetupQuadMaterial()
	{
		m_quadShaders = std::make_unique<Render::ShaderProgram>();

		auto vertexShader = std::make_unique<Render::ShaderBinary>();
		std::string errorText;
		if (!vertexShader->CompileFromFile(Render::ShaderType::VertexShader, "quad.vs", errorText))
		{
			SDE_LOG("Vertex shader compilation failed - %s", errorText.c_str());
		}
		auto fragmentShader = std::make_unique<Render::ShaderBinary>();
		if (!fragmentShader->CompileFromFile(Render::ShaderType::FragmentShader, "quad.fs", errorText))
		{
			SDE_LOG("Fragment shader compilation failed - %s", errorText.c_str());
		}

		if (!m_quadShaders->Create(*vertexShader, *fragmentShader, errorText))
		{
			SDE_LOG("Shader linkage failed - %s", errorText.c_str());
		}
	}

	void Renderer2D::PopulateInstanceBuffers()
	{
		SDE_PROF_EVENT();

		//static to avoid constant allocations
		static std::vector<glm::mat4> instanceTransforms;
		instanceTransforms.reserve(c_maxQuads);
		instanceTransforms.clear();
		static std::vector<glm::vec4> instanceColours;
		instanceColours.reserve(c_maxQuads);
		instanceColours.clear();

		for (const auto& q : m_quads)
		{
			glm::mat4 modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, glm::vec3(q.m_position, 0.0f));
			modelMat = glm::scale(modelMat, glm::vec3(q.m_size, 0.0f));
			instanceTransforms.push_back(modelMat);
			instanceColours.push_back(q.m_colour);
		}

		// copy the instance buffers to gpu
		m_quadInstanceTransforms.SetData(0, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
		m_quadInstanceColours.SetData(0, instanceColours.size() * sizeof(glm::vec4), instanceColours.data());
	}

	void Renderer2D::RenderAll(Render::Device& d)
	{
		SDE_PROF_EVENT();

		static bool s_firstFrame = true;
		if (s_firstFrame)
		{
			BuildQuadMesh();
			SetupQuadMaterial();
			s_firstFrame = false;
		}

		if (m_quads.size() == 0)
		{
			return;
		}

		// Sort the quads by texture to separate draw calls
		std::sort(m_quads.begin(), m_quads.end(), [](const Quad& q1, const Quad& q2) -> bool {
			return q1.m_texture.m_index < q2.m_texture.m_index;
			});

		PopulateInstanceBuffers();

		// global uniforms
		auto projectionMat = glm::ortho(0.0f, m_windowSize.x, 0.0f, m_windowSize.y);
		Render::UniformBuffer ub;
		ub.SetValue("ProjectionMat", projectionMat);

		// render state
		d.SetDepthState(true, false);		// enable z-test, disable write
		d.SetBackfaceCulling(true, true);	// backface culling, ccw order
		d.SetBlending(true);				// enable blending (we might want to do it manually instead)
		d.SetScissorEnabled(false);			// (don't) scissor me timbers

		d.BindShaderProgram(*m_quadShaders);

		// bind vertex array
		d.BindVertexArray(m_quadMesh->GetVertexArray());

		// set instance buffers. matrices must be set over multiple stream as part of vao
		d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 1, 4, 0, 4);
		d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 2, 4, sizeof(float) * 4, 4);
		d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 3, 4, sizeof(float) * 8, 4);
		d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceTransforms, 4, 4, sizeof(float) * 12, 4);
		d.BindInstanceBuffer(m_quadMesh->GetVertexArray(), m_quadInstanceColours, 5, 4, 0);

		// find the first and last quad with the same texture
		auto firstQuad = m_quads.begin();
		while (firstQuad != m_quads.end())
		{
			uint64_t texID = firstQuad->m_texture.m_index;
			auto lastQuad = std::find_if(firstQuad, m_quads.end(), [texID](const Quad& q) -> bool {
				return q.m_texture.m_index != texID;
				});

			// use the texture or our in built white texture
			smol::TextureHandle texture = firstQuad->m_texture.m_index != (uint64_t)-1 ? firstQuad->m_texture : smol::TextureHandle{ 0 };
			ub.SetSampler("DiffuseTexture", m_textures->GetTexture(texture)->GetHandle());
			d.SetUniforms(*m_quadShaders, ub);

			// somehow only draw firstQuad-lastQuad instances
			const auto& meshChunk = m_quadMesh->GetChunks()[0];
			auto instanceCount = (uint32_t)(lastQuad - firstQuad);
			uint32_t firstIndex = (uint32_t)(firstQuad - m_quads.begin());
			d.DrawPrimitivesInstanced(meshChunk.m_primitiveType, meshChunk.m_firstVertex, meshChunk.m_vertexCount, instanceCount, firstIndex);

			firstQuad = lastQuad;
		}
	}
}