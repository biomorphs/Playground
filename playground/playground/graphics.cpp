#include "graphics.h"
#include "kernel/log.h"
#include "core/system_enumerator.h"
#include "debug_gui/debug_gui_system.h"
#include "sde/script_system.h"
#include "sde/render_system.h"
#include <sol.hpp>

Graphics::Graphics()
{
}

Graphics::~Graphics()
{
}

bool Graphics::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_debugGui = (DebugGui::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_renderSystem = (SDE::RenderSystem*)systemEnumerator.GetSystem("Render");

	return true;
}

bool Graphics::Initialise()
{
	m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
	m_scriptSystem->Globals()["Graphics"]["SetClearColour"] = [this](float r, float g, float b) {
		m_renderSystem->SetClearColour(glm::vec4(r, g, b, 1.0f));
	};
	return true;
}

bool Graphics::Tick()
{
	return true;
}

void Graphics::Shutdown()
{
}