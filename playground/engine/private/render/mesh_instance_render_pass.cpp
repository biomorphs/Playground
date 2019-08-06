/*
SDLEngine
Matt Hoyle
*/
#include "mesh_instance_render_pass.h"
#include "device.h"
#include "material.h"
#include "shader_program.h"
#include "mesh.h"

namespace Render
{
	MeshInstanceRenderPass::MeshInstanceRenderPass()
	{

	}

	MeshInstanceRenderPass::~MeshInstanceRenderPass()
	{

	}

	void MeshInstanceRenderPass::Reset()
	{
		m_instances.Clear();
	}

	void MeshInstanceRenderPass::ApplyRenderState(Device& d)
	{
		d.SetDepthState(m_renderState.m_depthTestEnabled, m_renderState.m_depthWritingEnabled);
		d.SetBackfaceCulling(m_renderState.m_backfaceCullEnabled, m_renderState.m_frontFaceOrderCCW);
		d.SetBlending(m_renderState.m_blendingEnabled);
		d.SetScissorEnabled(m_renderState.m_scissorEnabled);
	}

	void MeshInstanceRenderPass::ApplyUniforms(Device& d, const ShaderProgram& p, const UniformBuffer& uniforms)
	{
		for (const auto& it : uniforms.Vec4Values())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			d.SetUniformValue(uniformHandle, it.second);
		}

		for (const auto& it : uniforms.Mat4Values())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			d.SetUniformValue(uniformHandle, it.second);
		}

		uint32_t textureUnit = 0;
		for (const auto& it : uniforms.Samplers())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			d.SetSampler(uniformHandle, it.second, textureUnit++);
		}

		for (const auto& it : uniforms.ArraySamplers())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			d.SetArraySampler(uniformHandle, it.second, textureUnit++);
		}
	}

	void MeshInstanceRenderPass::AddInstance(const Mesh* mesh)
	{
		m_instances.AddInstance(mesh, 0, (uint32_t)mesh->GetChunks().size());
	}

	void MeshInstanceRenderPass::AddInstance(const Mesh* mesh, UniformBuffer&& instanceUniforms)
	{
		m_instances.AddInstance(mesh, 0, (uint32_t)mesh->GetChunks().size(), std::move(instanceUniforms));
	}

	void MeshInstanceRenderPass::AddInstance(const Mesh* mesh, UniformBuffer&& instanceUniforms, uint32_t startChunk, uint32_t endChunk)
	{
		m_instances.AddInstance(mesh, startChunk, endChunk, std::move(instanceUniforms));
	}

	void MeshInstanceRenderPass::RenderAll(Device& device)
	{
		// Shadow current state to save driver overhead
		const Mesh* currentMesh = nullptr;
		const ShaderProgram* currentProgram = nullptr;
		const Material* currentMaterial = nullptr;

		ApplyRenderState(device);	// Apply any global render state
		for (const auto& it : m_instances)
		{
			const Mesh* theMesh = it.GetMesh();
			if (theMesh == nullptr)
			{
				continue;	// Should never occur, but just to be safe
			}
			const Material* theMaterial = theMesh->GetMaterial();
			const ShaderProgram* theShader = theMaterial->GetShaderProgram();

			if (theShader != currentProgram)
			{
				currentProgram = theShader;
				device.BindShaderProgram(*theShader);
			}

			if (theMaterial != currentMaterial)
			{
				currentMaterial = theMaterial;
				ApplyUniforms(device, *currentProgram, theMaterial->GetUniforms());
			}

			// apply instance uniforms
			ApplyUniforms(device, *currentProgram, it.GetUniforms());

			if (theMesh != currentMesh)
			{
				currentMesh = theMesh;
				device.BindVertexArray(theMesh->GetVertexArray());
			}

			for (uint32_t c = it.GetStartChunk(); c < it.GetEndChunk(); ++c)
			{
				const auto& thisChunk = theMesh->GetChunks()[c];
				device.DrawPrimitives(thisChunk.m_primitiveType, thisChunk.m_firstVertex, thisChunk.m_vertexCount);
			}
		}
	}
}