#pragma once
#include "math/glm_headers.h"

namespace smol
{
	class Light
	{
	public:
		glm::vec4 m_colour;		// w = ambient strength
		glm::vec3 m_position;
	};
}