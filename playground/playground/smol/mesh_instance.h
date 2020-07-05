#pragma once
#include <stdint.h>
#include "math/glm_headers.h"
#include "texture_manager.h"
#include "mesh_manager.h"

namespace smol
{
	struct MeshInstance
	{
		uint64_t m_sortKey;
		glm::mat4 m_transform;
		glm::vec4 m_colour;				// todo: how do we make this generic?
		smol::TextureHandle m_texture;	// todo: generic samplers
		MeshHandle m_mesh;
	};
}