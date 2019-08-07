/*
SDLEngine
Matt Hoyle
*/
#include "uniform_buffer.h"
#include "texture.h"
#include "core/string_hashing.h"

namespace Render
{
	void UniformBuffer::SetValue(std::string name, const glm::mat4& value)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_mat4Values[hash] = { name, value };
	}

	void UniformBuffer::SetValue(std::string name, const glm::vec4& value)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_vec4Values[hash] = { name, value };
	}

	void UniformBuffer::SetSampler(std::string name, uint32_t handle)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_textureSamplers[hash] = { name, handle };
	}

	void UniformBuffer::SetArraySampler(std::string name, uint32_t handle)
	{
		const uint32_t hash = Core::StringHashing::GetHash(name.c_str());
		m_textureArraySamplers[hash] = { name, handle };
	}
}