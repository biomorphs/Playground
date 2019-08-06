#pragma once
#include "core/system.h"
#include "core/timer.h"
#include "math/glm_headers.h"
#include <vector>
#include <string>

namespace DebugGui
{
	class DebugGuiSystem;
}

namespace SDE
{
	class ScriptSystem;
	class RenderSystem;
}

namespace Render
{
	class RenderPass;
	class Mesh;
}

class Graphics : public Core::ISystem
{
public:
	Graphics();
	virtual ~Graphics();
	virtual bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	virtual bool Initialise();
	virtual bool PostInit();
	virtual bool Tick();
	virtual void Shutdown();
	void DrawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 colour);
private:
	struct Quad;
	class RenderPass2D;
	std::vector<Quad> m_quads;
	std::unique_ptr<RenderPass2D> m_renderPass;
	DebugGui::DebugGuiSystem* m_debugGui = nullptr;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
	SDE::RenderSystem* m_renderSystem = nullptr;
};