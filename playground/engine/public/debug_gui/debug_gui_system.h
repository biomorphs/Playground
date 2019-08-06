/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "core/system.h"
#include "kernel/base_types.h"
#include "math/glm_headers.h"
#include <memory>

namespace SDE
{
	class RenderSystem;
}

namespace Render
{
	class Texture;
}

namespace DebugGui
{
	class ImguiSdlGL3RenderPass;
	class GraphDataBuffer;
	class DebugGuiSystem : public Core::ISystem
	{
	public:
		DebugGuiSystem();
		virtual ~DebugGuiSystem();

		virtual bool PreInit(Core::ISystemEnumerator& systemEnumerator);
		virtual bool Initialise() override;
		virtual bool PostInit() override;
		virtual bool Tick() override;
		virtual void Shutdown() override;

		void BeginWindow(bool& windowOpen, const char* windowName, glm::vec2 size=glm::vec2(-1.f));
		void EndWindow();
		void Text(const char* txt);
		bool Button(const char* txt);
		void Separator();
		void Image(Render::Texture& src, glm::vec2 size, glm::vec2 uv0 = glm::vec2(0.0f,0.0f), glm::vec2 uv1 = glm::vec2(1.0f,1.0f));
		void GraphLines(const char* label, glm::vec2 size, GraphDataBuffer& buffer);
		void GraphHistogram(const char* label, glm::vec2 size, GraphDataBuffer& buffer);
		bool Checkbox(const char* text, bool* val);
		bool DragFloat(const char* label, float& f, float step = 1.0f, float min = 0.0f, float max = 0.0f);
		bool DragVector(const char* label, glm::vec4& v, float step = 1.0f, float min = 0.0f, float max = 0.0f);
		bool DragVector(const char* label, glm::vec3& v, float step = 1.0f, float min = 0.0f, float max = 0.0f);
		bool ColourEdit(const char* label, glm::vec4& c, bool showAlpha = true);

	private:
		SDE::RenderSystem* m_renderSystem;
		std::unique_ptr<ImguiSdlGL3RenderPass> m_imguiPass;
	};
}