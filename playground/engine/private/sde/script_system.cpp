/*
SDLEngine
Matt Hoyle
*/
#include "script_system.h"
#include "kernel/file_io.h"
#include <sol.hpp>

namespace SDE
{
	ScriptSystem::ScriptSystem()
	{
	}

	ScriptSystem::~ScriptSystem()
	{
	}

	void ScriptSystem::OpenDefaultLibraries(sol::state& state)
	{
		state.open_libraries(sol::lib::base, sol::lib::math);
	}

	void ScriptSystem::RunScriptFromFile(const char* filename)
	{
		std::string scriptText;
		if (Kernel::FileIO::LoadTextFromFile(filename, scriptText))
		{
			RunScript(scriptText.data());
		}
	}

	bool ScriptSystem::RunScriptFromFile(const char* filename, std::string& errorText) noexcept
	{
		try
		{
			RunScriptFromFile(filename);
			return true;
		}
		catch (const sol::error& err)
		{
			errorText = err.what();
			return false;
		}
	}

	void ScriptSystem::RunScript(const char* scriptSource)
	{
		m_globalState->script(scriptSource);
	}

	bool ScriptSystem::RunScript(const char* scriptSource, std::string& errorText) noexcept
	{
		try 
		{
			RunScript(scriptSource);
			return true;
		}
		catch (const sol::error& err) 
		{
			errorText = err.what();
			return false;
		}
	}

	bool ScriptSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
	{
		m_globalState = std::make_unique<sol::state>();
		OpenDefaultLibraries(*m_globalState);
		
		return true;
	}

	bool ScriptSystem::Tick()
	{
		m_globalState->collect_garbage();
		return true;
	}

	void ScriptSystem::Shutdown()
	{
		m_globalState = nullptr;
	}
}