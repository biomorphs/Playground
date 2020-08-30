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
		glm::mat4 m_transform;
		glm::vec4 m_colour;
		smol::ShaderHandle m_shader;
		const Render::Mesh* m_mesh;
	};
}