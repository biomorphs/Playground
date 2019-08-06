#pragma once
#include "core/system.h"
#include "core/timer.h"
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

class Graphics : public Core::ISystem
{
public:
	Graphics();
	virtual ~Graphics();
	virtual bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	virtual bool Initialise();
	virtual bool Tick();
	virtual void Shutdown();
private:
	DebugGui::DebugGuiSystem* m_debugGui = nullptr;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
	SDE::RenderSystem* m_renderSystem = nullptr;
};