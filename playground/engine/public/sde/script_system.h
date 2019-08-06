/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "core/system.h"
#include <string>

namespace sol
{
	class state;
	struct protected_function_result;
}

namespace SDE
{
	// Handles LUA via sol2. Exposes sol2 state directly since I cba wrapping it
	// (and the interface is nice enough)
	class ScriptSystem : public Core::ISystem
	{
	public:
		ScriptSystem();
		virtual ~ScriptSystem();

		bool PreInit(Core::ISystemEnumerator& systemEnumerator);
		bool Tick();
		void Shutdown();

		sol::state& Globals() { return *m_globalState; }
		
		void RunScriptFromFile(const char* filename);	// can throw sol::error
		bool RunScriptFromFile(const char* filename, std::string& errorText) noexcept;
		void RunScript(const char* scriptSource);	// can throw sol::error
		bool RunScript(const char* scriptSource, std::string& errorText) noexcept;

	private:
		void OpenDefaultLibraries(sol::state& state);
		std::unique_ptr<sol::state> m_globalState;
	};
}