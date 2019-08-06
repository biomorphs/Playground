/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "core/system.h"
#include <sol.hpp>		// we expose sol directly, so this doesn't hurt

namespace SDE
{
	class ScriptSystem;

	// Calls scripts that populate the global Config namespace in lua
	// Getters return tables inside the config namespace
	// e.g. GetConfig("Render") gets lua['Config']['Render']
	class ConfigSystem : public Core::ISystem
	{
	public:
		ConfigSystem();
		virtual ~ConfigSystem();

		void LoadConfigFile(const char* path);
		bool PreInit(Core::ISystemEnumerator& systemEnumerator);
		void PreShutdown();

		const sol::table Values() const;

	private:
		ScriptSystem* m_scriptSystem;
	};
}