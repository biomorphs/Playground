#include "playground.h"
#include "kernel/time.h"
#include "kernel/log.h"
#include "core/system_enumerator.h"
#include "debug_gui/debug_gui_system.h"
#include "debug_gui/debug_gui_menubar.h"
#include "sde/script_system.h"
#include "core/profiler.h"
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
	SDE_PROF_EVENT();
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
	SDE_PROF_EVENT();
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
	SDE_PROF_EVENT();
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
	SDE_PROF_EVENT();
	ShutdownScript();
	m_scriptErrorText = "";
	if (m_scriptSystem->RunScriptFromFile(m_scriptPath.c_str(), m_scriptErrorText))
	{
		InitScript();
	}
	else
	{
		SDE_LOG("Lua Error - %s", m_scriptErrorText.c_str());
	}
}

DebugGui::MenuBar g_menuBar;
bool g_keepRunning = true;
bool g_pauseScriptDelta = false;

bool Playground::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	SDE_PROF_EVENT();
	m_debugGui = (DebugGui::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_lastFrameTime = m_timer.GetSeconds();

	auto& fileMenu = g_menuBar.AddSubmenu(ICON_FK_FILE_O " File");
	fileMenu.AddItem("Exit", []() { g_keepRunning = false; });

	auto& scriptMenu = g_menuBar.AddSubmenu(ICON_FK_COG " Scripts");
	scriptMenu.AddItem("Reload", [this]() {ReloadScript(); }, "R");
	scriptMenu.AddItem("Toggle Paused", [] { g_pauseScriptDelta = !g_pauseScriptDelta; });

	return true;
}

bool Playground::Tick()
{
	SDE_PROF_EVENT();
	static bool s_firstTick = true;
	if (s_firstTick)
	{
		s_firstTick = false;
		ReloadScript();
	}

	m_debugGui->MainMenuBar(g_menuBar);

	double thisFrameTime = m_timer.GetSeconds();
	if (!g_pauseScriptDelta)
	{
		m_deltaTime = thisFrameTime - m_lastFrameTime;
	}
	else
	{
		m_deltaTime = 0.0f;
	}
	TickScript();
	m_lastFrameTime = thisFrameTime;

	return g_keepRunning;
}

void Playground::Shutdown()
{
	SDE_PROF_EVENT();
	ShutdownScript();
}