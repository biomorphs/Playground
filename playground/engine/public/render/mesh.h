/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "vertex_array.h"
#include "device.h"	// move primitive type from here!
#include "render_buffer.h"
#include "material.h"
#include <memory>

namespace Render
{
	class Material;

	struct MeshChunk
	{
		MeshChunk() 
			: m_firstVertex(0), m_vertexCount(0), m_primitiveType(Render::PrimitiveType::Triangles) { }
		MeshChunk(uint32_t fv, uint32_t count, Render::PrimitiveType primitive) 
			: m_firstVertex(fv), m_vertexCount(count), m_primitiveType(primitive) { }
		uint32_t m_firstVertex;
		uint32_t m_vertexCount;
		Render::PrimitiveType m_primitiveType;
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		void Destroy();

		inline const Material& GetMaterial() const					{ return m_material; }
		inline const std::vector<RenderBuffer>& GetStreams() const	{ return m_vertexStreams; }
		inline const VertexArray& GetVertexArray() const			{ return m_vertices; }
		inline const std::vector<MeshChunk>& GetChunks() const		{ return m_chunks; }
		inline Material& GetMaterial() { return m_material; }
		inline std::vector<RenderBuffer>& GetStreams()				{ return m_vertexStreams; }
		inline VertexArray& GetVertexArray()						{ return m_vertices; }		
		inline std::vector<MeshChunk>& GetChunks()					{ return m_chunks; }

	private:
		VertexArray m_vertices;
		Material m_material;
		std::vector<RenderBuffer> m_vertexStreams;
		std::vector<MeshChunk> m_chunks;
	};
}