/*
SDLEngine
Matt Hoyle
*/
#include "config_system.h"
#include "script_system.h"
#include "kernel/file_io.h"
#include "kernel/log.h"
#include "core/system_enumerator.h"
#include "sde/script_system.h"
#include <sol.hpp>

namespace SDE
{
	ConfigSystem::ConfigSystem()
		: m_scriptSystem(nullptr)
	{
	}

	ConfigSystem::~ConfigSystem()
	{
	}

	const sol::table ConfigSystem::Values() const
	{
		return m_scriptSystem->Globals()["Config"];
	}

	void ConfigSystem::LoadConfigFile(const char* path)
	{
		std::string errorText;
		if (!m_scriptSystem->RunScriptFromFile(path, errorText))
		{
			SDE_LOG("Loading config '%s' failed -\n\t%s", path, errorText.c_str());
		}
	}

	bool ConfigSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
	{
		m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
		m_scriptSystem->Globals().create_named_table("Config");
		LoadConfigFile("config.lua");

		return true;
	}

	void ConfigSystem::PreShutdown()
	{
		m_scriptSystem = nullptr;
	}
}