#pragma once
#include "math/glm_headers.h"

namespace smol
{
	class Light
	{
	public:
		glm::vec4 m_colour;		// w = ambient strength
		glm::vec4 m_position;	// w = 0, light is directional : w=1, point light
		glm::vec3 m_attenuation;	// constant, linear, quadratic
	};
}