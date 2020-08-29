#pragma once

namespace sol
{
	class state;
}

namespace DebugGui
{
	class DebugGuiSystem;
}

namespace DebugGuiScriptBinding
{
	void Go(DebugGui::DebugGuiSystem* system, sol::state& context);
}