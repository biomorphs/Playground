#pragma once
#include "core/system.h"

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
	DebugGui::DebugGuiSystem* m_debugGui = nullptr;
	SDE::ScriptSystem* m_scriptSystem = nullptr;
};