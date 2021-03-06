#include "render_target_blitter.h"
#include "render/frame_buffer.h"
#include "render/shader_program.h"
#include "render/device.h"
#include "render/material.h"
#include "render/mesh.h"
#include "render/mesh_builder.h"
#include "render/texture.h"
#include "core/profiler.h"
#include "material_helpers.h"

namespace smol
{
	RenderTargetBlitter::RenderTargetBlitter()
	{
		// screen space quad
		Render::MeshBuilder builder;
		builder.AddVertexStream(2);
		builder.BeginChunk();
		const glm::vec2 v0 = { -1.0f,-1.0f };
		const glm::vec2 v1 = { 1.0f,1.0f };
		builder.BeginTriangle();
		builder.SetStreamData(0, {v0.x,v1.y}, { v0.x,v0.y }, { v1.x,v0.y } );
		builder.EndTriangle();
		builder.BeginTriangle();
		builder.SetStreamData(0, { v0.x,v1.y }, { v1.x,v0.y } , { v1.x,v1.y } );
		builder.EndTriangle();
		builder.EndChunk();

		m_quadMesh = std::make_unique<Render::Mesh>();
		builder.CreateMesh(*m_quadMesh);
		builder.CreateVertexArray(*m_quadMesh);
	}

	void RenderTargetBlitter::TargetToBackbuffer(Render::Device& d, const Render::FrameBuffer& src, Render::ShaderProgram& shader, glm::ivec2 dimensions)
	{
		SDE_PROF_EVENT();
		if (src.GetHandle() == -1 || shader.GetHandle() == -1 || src.GetColourAttachmentCount() == 0)
		{
			return;
		}
		d.DrawToBackbuffer();
		d.SetViewport(glm::ivec2(0), dimensions);
		d.BindShaderProgram(shader);
		d.BindVertexArray(m_quadMesh->GetVertexArray());
		auto sampler = shader.GetUniformHandle("SourceTexture");
		if (sampler != -1)
		{
			d.SetSampler(sampler, src.GetColourAttachment(0).GetHandle(), 0);
		}
		for (const auto& chunk : m_quadMesh->GetChunks())
		{
			d.DrawPrimitives(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount);
		}
	}

	void RenderTargetBlitter::TargetToTarget(Render::Device& d, const Render::FrameBuffer& src, Render::FrameBuffer& target, Render::ShaderProgram& shader)
	{
		SDE_PROF_EVENT();
		if (src.GetHandle() == -1 || target.GetHandle() == -1 || shader.GetHandle() == -1)
		{
			return;
		}
		d.DrawToFramebuffer(target);
		d.SetViewport(glm::ivec2(0), target.Dimensions());
		d.BindShaderProgram(shader);
		d.BindVertexArray(m_quadMesh->GetVertexArray());
		for (const auto& chunk : m_quadMesh->GetChunks())
		{
			d.DrawPrimitives(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount);
		}
	}
}