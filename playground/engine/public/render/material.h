/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "uniform_buffer.h"
#include "kernel/base_types.h"
#include <memory>

namespace Render
{
	class ShaderProgram;

	class Material
	{
	public:
		Material();
		~Material();

		inline UniformBuffer& GetUniforms()							{ return m_uniforms; }
		inline const UniformBuffer& GetUniforms() const				{ return m_uniforms; }

	private:
		UniformBuffer m_uniforms;
	};
}