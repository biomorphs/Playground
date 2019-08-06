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
}

class Playground : public Core::ISystem
{
public:
	Playground();
	virtual ~Playground();
	virtual bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	virtual bool Tick();
	virtual void Shutdown();
private:
	void ReloadScript();
	void InitScript();
	void TickScript();
	void ShutdownScript();
	DebugGui::DebugGuiSystem* m_debugGui = nullptr;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
	std::string m_scriptPath = "playground.lua";
	std::string m_scriptErrorText;
	Core::Timer m_timer;
	double m_lastFrameTime = 0.0;
	double m_deltaTime = 0.0;
};