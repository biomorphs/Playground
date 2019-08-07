/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "math/glm_headers.h"
#include <unordered_map>
#include <string>

namespace Render
{
	class Texture;

	// Contains a set of name -> value pairs for a set of uniforms + samplers
	class UniformBuffer
	{
	public:
		UniformBuffer() = default;
		~UniformBuffer() = default;
		UniformBuffer(const UniformBuffer& other) = default;
		UniformBuffer(UniformBuffer&& other) = default;

		template <class T>
		struct Uniform {
			std::string m_name;
			T m_value;
		};
		using Vec4Uniforms = std::unordered_map<uint32_t, Uniform<glm::vec4>>;
		using Mat4Uniforms = std::unordered_map<uint32_t, Uniform<glm::mat4>>;
		using SamplerUniforms = std::unordered_map<uint32_t, Uniform<uint32_t>>;

		void SetValue(std::string name, const glm::vec4& value);
		void SetValue(std::string name, const glm::mat4& value);
		void SetSampler(std::string name, uint32_t handle);
		void SetArraySampler(std::string name, uint32_t handle);

		const Vec4Uniforms& Vec4Values() const { return m_vec4Values; }
		const Mat4Uniforms& Mat4Values() const { return m_mat4Values; }
		const SamplerUniforms& Samplers() const { return m_textureSamplers; }
		const SamplerUniforms& ArraySamplers() const { return m_textureArraySamplers; }
		
	private:
		Vec4Uniforms m_vec4Values;
		Mat4Uniforms m_mat4Values;
		SamplerUniforms m_textureSamplers;
		SamplerUniforms m_textureArraySamplers;
	};
}