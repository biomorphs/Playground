#include "playground.h"
#include "kernel/time.h"
#include "kernel/log.h"
#include "core/system_enumerator.h"
#include "debug_gui/debug_gui_system.h"
#include "sde/script_system.h"
#include "model_asset.h"
#include <sol.hpp>

Playground::Playground()
{
	srand((uint32_t)Kernel::Time::HighPerformanceCounterTicks());
}

Playground::~Playground()
{
}

void Playground::InitScript()
{
	try
	{
		if (m_scriptSystem->Globals()["g_playground"] != nullptr)
		{
			sol::function initFn = m_scriptSystem->Globals()["g_playground"]["Init"];
			initFn();
		}
		else
		{
			m_scriptErrorText = "Error: g_playground global not found";
		}
	}
	catch (const sol::error& err)
	{
		SDE_LOG("Lua Error - %s", err.what());
		ShutdownScript();
	}
}

void Playground::TickScript()
{
	try
	{
		m_scriptSystem->Globals()["Playground"].get_or_create<sol::table>();
		m_scriptSystem->Globals()["Playground"]["DeltaTime"] = m_deltaTime;
		if (m_scriptSystem->Globals()["g_playground"] != nullptr)
		{
			sol::function tickFn = m_scriptSystem->Globals()["g_playground"]["Tick"];
			tickFn();
		}
	}
	catch (const sol::error& err)
	{
		SDE_LOG("Lua Error - %s", err.what());
		ShutdownScript();
	}
}

void Playground::ShutdownScript()
{
	try
	{
		if (m_scriptSystem->Globals()["g_playground"] != nullptr)
		{
			sol::function shutdownFn = m_scriptSystem->Globals()["g_playground"]["Shutdown"];
			shutdownFn();
		}
		m_scriptSystem->Globals()["g_playground"] = nullptr;
	}
	catch (const sol::error& err)
	{
		SDE_LOG("Lua Error - %s", err.what());
	}
}

void Playground::ReloadScript()
{
	ShutdownScript();
	m_scriptErrorText = "";
	if (m_scriptSystem->RunScriptFromFile(m_scriptPath.c_str(), m_scriptErrorText))
	{
		InitScript();
	}
}

bool Playground::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_debugGui = (DebugGui::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_lastFrameTime = m_timer.GetSeconds();

	auto loadedModel = Model::Loader::Load("cottage\\cottage_blender.fbx");

	return true;
}

bool Playground::Tick()
{
	bool forceOpen = true;
	m_debugGui->BeginWindow(forceOpen, "Script");
	if (m_debugGui->Button("Reload"))
	{
		ReloadScript();
	}
	if (m_scriptErrorText.length() > 0)
	{
		m_debugGui->Separator();
		m_debugGui->Text(m_scriptErrorText.c_str());
	}
	m_debugGui->EndWindow();

	double thisFrameTime = m_timer.GetSeconds();
	m_deltaTime = thisFrameTime - m_lastFrameTime;
	TickScript();
	m_lastFrameTime = thisFrameTime;

	return true;
}

void Playground::Shutdown()
{
	ShutdownScript();
}