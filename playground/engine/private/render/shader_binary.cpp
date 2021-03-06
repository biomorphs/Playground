/*
SDLEngine
Matt Hoyle
*/
#include "shader_binary.h"
#include "utils.h"
#include "kernel/file_io.h"
#include "core/profiler.h"

namespace Render
{
	ShaderBinary::ShaderBinary()
		: m_handle(0)
	{
	}

	ShaderBinary::~ShaderBinary()
	{
		Destroy();
	}

	inline uint32_t ShaderBinary::TranslateShaderType(ShaderType type) const
	{
		switch (type)
		{
		case ShaderType::VertexShader:
			return GL_VERTEX_SHADER;
		case ShaderType::FragmentShader:
			return GL_FRAGMENT_SHADER;
		default:
			return -1;
		}
	}

	bool ShaderBinary::CompileFromBuffer(ShaderType type, const std::string& buffer, std::string& resultText)
	{
		return CompileSource(type, buffer, resultText);
	}

	bool ShaderBinary::CompileSource(ShaderType type, const std::string& src, std::string& resultText)
	{
		SDE_PROF_EVENT();

		uint32_t shaderType = TranslateShaderType(type);
		SDE_ASSERT(shaderType != -1);

		m_handle = glCreateShader(shaderType);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateShader");
		SDE_RENDER_ASSERT(m_handle != 0);

		char const* srcPtr = src.c_str();
		glShaderSource(m_handle, 1, &srcPtr, nullptr);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glShaderSource");

		glCompileShader(m_handle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCompileShader");

		// check for compile errors
		int32_t compileResult = 0, resultLogSize = 0;
		glGetShaderiv(m_handle, GL_COMPILE_STATUS, &compileResult);
		glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &resultLogSize);

		char errorLogResult[2048] = { '\0' };
		SDE_ASSERT(resultLogSize < sizeof(errorLogResult));

		glGetShaderInfoLog(m_handle, resultLogSize, nullptr, errorLogResult);
		resultText = errorLogResult;

		m_type = type;

		return compileResult == GL_TRUE;
	}

	void ParseIncludes(std::string& src)
	{
		SDE_PROF_EVENT();

		std::vector<std::string> includedFiles;		// make sure we don't include multiple times (also avoids circular dependencies)
		const std::string c_includeDirectivePrefix = "#pragma sde include";
		size_t foundInclude = 0;
		while ((foundInclude = src.find(c_includeDirectivePrefix, foundInclude)) != std::string::npos)
		{
			size_t firstNewline = src.find("\n", foundInclude);
			size_t firstQuote = src.find("\"", foundInclude + c_includeDirectivePrefix.length());
			if (firstQuote == std::string::npos)
			{
				SDE_LOG("Malformed include path encountered (no quotes)\n%s", src.c_str()); 
				return;
			}
			if (firstNewline != std::string::npos && firstNewline < firstQuote)
			{
				SDE_LOG("Malformed include path encountered (newline before quotes)\n%s", src.c_str());
				return;
			}
			size_t secondQuote = src.find("\"", firstQuote + 1);
			if (secondQuote == std::string::npos)
			{
				SDE_LOG("Malformed include path encountered (no closing quote)\n%s", src.c_str());
				return;
			}
			if (firstNewline != std::string::npos && firstNewline < secondQuote)
			{
				SDE_LOG("Malformed include path encountered (newline in path?)\n%s", src.c_str());
				return;
			}
			if (secondQuote > firstQuote)
			{
				std::string includePath = src.substr(firstQuote + 1, secondQuote - firstQuote - 1);
				if (std::find(includedFiles.begin(), includedFiles.end(), includePath) == includedFiles.end())
				{
					std::string includeSrc;
					if (!Kernel::FileIO::LoadTextFromFile(includePath.c_str(), includeSrc))
					{
						SDE_ASSERT(false, "Failed to include shader source from %s", includeSrc.c_str());
						return;
					}
					std::string prefix = src.substr(0, secondQuote + 1);
					std::string postfix = src.substr(secondQuote + 1);
					src = prefix + "\n" + includeSrc + postfix;
					includedFiles.push_back(includePath);
				}
			}
			foundInclude = secondQuote;
		}
	}

	bool ShaderBinary::CompileFromFile(ShaderType type, const char* srcLocation, std::string& resultText)
	{
		char debugName[1024] = { '\0' };
		sprintf_s(debugName, "Render::ShaderBinary::CompileFromFile(\"%s\")", srcLocation);
		SDE_PROF_EVENT_DYN(debugName);

		std::string shaderSource;
		if (!Kernel::FileIO::LoadTextFromFile(srcLocation, shaderSource))
		{
			SDE_ASSERT(false, "Failed to load shader source from %s", srcLocation);
			return false;
		}

		ParseIncludes(shaderSource);
		return CompileSource(type, shaderSource, resultText);
	}

	void ShaderBinary::Destroy()
	{
		SDE_PROF_EVENT();
		if (m_handle != 0)
		{
			glDeleteShader(m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS("glDeleteShader");
		}

		m_handle = 0;
	}
}