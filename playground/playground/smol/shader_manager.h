#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace Render
{
	class ShaderProgram;
}

namespace smol
{
	struct ShaderHandle
	{
		uint16_t m_index = -1;
		static ShaderHandle Invalid() { return { (uint16_t)-1 }; };
	};

	class ShaderManager
	{
	public:
		ShaderManager() = default;
		~ShaderManager() = default;
		ShaderManager(const ShaderManager&) = delete;
		ShaderManager(ShaderManager&&) = delete;

		ShaderHandle LoadShader(const char* name, const char* vsPath, const char* fsPath);
		Render::ShaderProgram* GetShader(const ShaderHandle& h);

		void ReloadAll();

	private:
		struct ShaderDesc {
			std::unique_ptr<Render::ShaderProgram> m_shader;
			std::string m_name;
			std::string m_vsPath;
			std::string m_fsPath;
		};
		std::vector<ShaderDesc> m_shaders;
	};
}