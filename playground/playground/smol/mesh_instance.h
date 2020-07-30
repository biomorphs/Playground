#pragma once
#include <stdint.h>
#include "math/glm_headers.h"
#include "texture_manager.h"
#include "shader_manager.h"

namespace Render
{
	class Mesh;
}

namespace smol
{
	struct MeshInstance
	{
		uint64_t m_sortKey;
		glm::mat4 m_transform;
		glm::vec4 m_colour;				// todo: how do we make this generic?
		smol::TextureHandle m_texture;	// todo: generic samplers
		smol::TextureHandle m_normalTexture;
		smol::TextureHandle m_specularTexture;
		smol::ShaderHandle m_shader;
		const Render::Mesh* m_mesh;
	};
}