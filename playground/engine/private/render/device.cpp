/*
SDLEngine
Matt Hoyle
*/

#include "device.h"
#include "window.h"
#include "utils.h"
#include "vertex_array.h"
#include "shader_program.h"
#include "render_buffer.h"
#include <SDL.h>
#include <glew.h>
#include "math/glm_headers.h"
#include "uniform_buffer.h"

namespace Render
{
	Device::Device(Window& theWindow)
		: m_window( theWindow )
	{
		SDL_Window* windowHandle = theWindow.GetWindowHandle();
		SDE_RENDER_ASSERT(windowHandle);

		// Request opengl 4.3 context.
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

		// Always doublebuffer + 24 bit depth
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		m_context = SDL_GL_CreateContext(windowHandle);
		SDE_RENDER_ASSERT(m_context);

		SDL_GL_SetSwapInterval(1);	// 0 = no vsync, 1 = vsync

		// glew initialises GL function pointers
		glewExperimental = true;		// must be set for core profile and above
		auto glewError = glewInit();
		SDE_ASSERT(glewError == GLEW_OK, "Failed to initialise glew: %s", glewGetErrorString(glewError));

		// glewInit can push INVALID_ENUM to the error stack (due to how extensions are checked)
		// pop everything off here
		auto glErrorPop = glGetError();
		while (glErrorPop == GL_INVALID_ENUM)
		{
			glErrorPop = glGetError();
		}
		SDE_RENDER_PROCESS_GL_ERRORS("Device Initialise");

		// Setting this here allows all point sprite shaders to set the sprite size
		// dynamically.
		glEnable(GL_PROGRAM_POINT_SIZE);
	}

	Device::~Device()
	{
		SDL_GL_DeleteContext(m_context);
		m_context = nullptr;
	}

	void Device::Present()
	{
		SDL_GL_SwapWindow(m_window.GetWindowHandle());
	}

	SDL_GLContext Device::GetGLContext()
	{
		return m_context;
	}

	inline uint32_t Device::TranslatePrimitiveType(PrimitiveType type) const
	{
		switch (type)
		{
		case PrimitiveType::Triangles:
			return GL_TRIANGLES;
		case PrimitiveType::Lines:
			return GL_LINES;
		case PrimitiveType::PointSprites:
			return GL_POINTS;
		default:
			return -1;
		}
	}

	void Device::SetScissorEnabled(bool enabled)
	{
		if (enabled)
		{
			glEnable(GL_SCISSOR_TEST);
		}
		else
		{
			glDisable(GL_SCISSOR_TEST);
		}
	}

	void Device::SetBlending(bool enabled)
	{
		if (enabled)
		{
			// Todo - separate
			glEnable(GL_BLEND);
			SDE_RENDER_PROCESS_GL_ERRORS("glEnable");
			glBlendEquation(GL_FUNC_ADD);
			SDE_RENDER_PROCESS_GL_ERRORS("glBlendEquation");
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			SDE_RENDER_PROCESS_GL_ERRORS("glBlendFunc");
		}
		else
		{
			glDisable(GL_BLEND);
			SDE_RENDER_PROCESS_GL_ERRORS("glDisable");
		}
	}

	void Device::SetBackfaceCulling(bool enabled, bool frontFaceCCW)
	{
		if (enabled)
		{
			glEnable(GL_CULL_FACE);
			SDE_RENDER_PROCESS_GL_ERRORS("glEnable");
		}
		else
		{
			glDisable(GL_CULL_FACE);
			SDE_RENDER_PROCESS_GL_ERRORS("glDisable");
		}

		glFrontFace(frontFaceCCW ? GL_CCW : GL_CW);
		SDE_RENDER_PROCESS_GL_ERRORS("glFrontFace");
	}

	void Device::SetDepthState(bool enabled, bool writeEnabled)
	{
		if (enabled)
		{
			glEnable(GL_DEPTH_TEST);
			SDE_RENDER_PROCESS_GL_ERRORS("glEnable");
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
			SDE_RENDER_PROCESS_GL_ERRORS("glDisable");
		}
		glDepthMask(writeEnabled);
		SDE_RENDER_PROCESS_GL_ERRORS("glDepthMask");
	}

	void Device::ClearColourDepthTarget(const glm::vec4& colour)
	{
		glClearColor(colour.r, colour.g, colour.b, colour.a);
		SDE_RENDER_PROCESS_GL_ERRORS("glClearColor");
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SDE_RENDER_PROCESS_GL_ERRORS("glClear");
	}

	void Device::SetArraySampler(uint32_t uniformHandle, uint32_t textureHandle, uint32_t textureUnit)
	{
		// gl is a bit wierd, it uses texture units bound to samplers, so we need to both
		// set the active texture for the specified unit, then bind the sampler to that unit
		SDE_ASSERT(uniformHandle != -1);
		SDE_ASSERT(textureHandle != 0);

		glActiveTexture(GL_TEXTURE0 + textureUnit);
		SDE_RENDER_PROCESS_GL_ERRORS("glActiveTexture");

		glBindTexture(GL_TEXTURE_2D_ARRAY, textureHandle);
		SDE_RENDER_PROCESS_GL_ERRORS("glBindTexture");

		glUniform1i(uniformHandle, textureUnit);
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform1i");
	}

	void Device::SetSampler(uint32_t uniformHandle, uint32_t textureHandle, uint32_t textureUnit)
	{
		// gl is a bit wierd, it uses texture units bound to samplers, so we need to both
		// set the active texture for the specified unit, then bind the sampler to that unit
		SDE_ASSERT(uniformHandle != -1);
		SDE_ASSERT(textureHandle != 0);

		glActiveTexture(GL_TEXTURE0 + textureUnit);
		SDE_RENDER_PROCESS_GL_ERRORS("glActiveTexture");

		glBindTexture(GL_TEXTURE_2D, textureHandle);
		SDE_RENDER_PROCESS_GL_ERRORS("glBindTexture");

		glUniform1i(uniformHandle, textureUnit);
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform1i");
	}

	void Device::SetUniformValue(uint32_t uniformHandle, const glm::mat4& matrix)
	{
		SDE_ASSERT(uniformHandle != -1);
		glUniformMatrix4fv(uniformHandle, 1, GL_FALSE, glm::value_ptr(matrix));
		SDE_RENDER_PROCESS_GL_ERRORS("glUniformMatrix4fv");
	}

	void Device::SetUniformValue(uint32_t uniformHandle, const glm::vec4& val)
	{
		SDE_ASSERT(uniformHandle != -1);
		glUniform4fv(uniformHandle, 1, glm::value_ptr(val));
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform4fv");
	}

	void Device::BindShaderProgram(const ShaderProgram& program)
	{
		glUseProgram(program.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glUseProgram");
	}

	// vectorcount used to pass matrices (4x4 mat = 4 components, 4 vectorcount)
	void Device::BindInstanceBuffer(const VertexArray& srcArray, const RenderBuffer& buffer, int vertexLayoutSlot, int components, size_t offset, size_t vectorCount)
	{
		SDE_ASSERT(buffer.GetHandle() != 0);
		SDE_ASSERT(components <= 4);

		BindVertexArray(srcArray);

		glBindBuffer(GL_ARRAY_BUFFER, buffer.GetHandle());		// bind the vbo
		SDE_RENDER_PROCESS_GL_ERRORS("glBindBuffer");

		glEnableVertexAttribArray(vertexLayoutSlot);			//enable the slot
		SDE_RENDER_PROCESS_GL_ERRORS("glEnableVertexAttribArray");

		// send the data (we have to send it 4 components at a time)
		// always float, never normalised
		glVertexAttribPointer(vertexLayoutSlot, components, GL_FLOAT, GL_FALSE, components * sizeof(float) * vectorCount, (void*)offset);
		SDE_RENDER_PROCESS_GL_ERRORS("glVertexAttribPointer");

		glVertexAttribDivisor(vertexLayoutSlot, 1);
		SDE_RENDER_PROCESS_GL_ERRORS("glVertexAttribDivisor");

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		SDE_RENDER_PROCESS_GL_ERRORS("glBindBuffer");
	}

	void Device::BindVertexArray(const VertexArray& srcArray)
	{
		SDE_ASSERT(srcArray.GetHandle() != 0);
		glBindVertexArray(srcArray.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glBindVertexArray");
	}

	void Device::DrawPrimitivesInstanced(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstInstance)
	{
		auto primitiveType = TranslatePrimitiveType(primitive);
		SDE_ASSERT(primitiveType != -1);

		glDrawArraysInstancedBaseInstance(primitiveType, vertexStart, vertexCount, instanceCount, firstInstance);
		SDE_RENDER_PROCESS_GL_ERRORS("glDrawArraysInstanced");
	}

	void Device::DrawPrimitives(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount)
	{
		auto primitiveType = TranslatePrimitiveType(primitive);
		SDE_ASSERT(primitiveType != -1);

		glDrawArrays(primitiveType, vertexStart, vertexCount);
		SDE_RENDER_PROCESS_GL_ERRORS("glDrawArrays");
	}

	void Device::SetUniforms(const ShaderProgram& p, const UniformBuffer& uniforms)
	{
		for (const auto& it : uniforms.Vec4Values())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			if(uniformHandle != -1)
				SetUniformValue(uniformHandle, it.second);
		}

		for (const auto& it : uniforms.Mat4Values())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			if (uniformHandle != -1)
				SetUniformValue(uniformHandle, it.second);
		}

		uint32_t textureUnit = 0;
		for (const auto& it : uniforms.Samplers())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			if (uniformHandle != -1)
				SetSampler(uniformHandle, it.second, textureUnit++);
		}

		for (const auto& it : uniforms.ArraySamplers())
		{
			// Find the uniform handle in the program
			auto uniformHandle = p.GetUniformHandle(it.first);
			if (uniformHandle != -1)
				SetArraySampler(uniformHandle, it.second, textureUnit++);
		}
	}
}