#include "sde/render_system.h"
#include "sde/job_system.h"
#include "sde/script_system.h"
#include "sde/event_system.h"
#include "sde/config_system.h"
#include "debug_gui/debug_gui_system.h"
#include "engine/engine_startup.h"
#include "input/input_system.h"
#include "core/system_registrar.h"
#include "playground.h"

class SystemRegistration : public Engine::IAppSystemRegistrar
{
public:
	void RegisterSystems(Core::ISystemRegistrar& systemManager)
	{
		systemManager.RegisterSystem("Events", new SDE::EventSystem());
		systemManager.RegisterSystem("Jobs", new SDE::JobSystem());
		systemManager.RegisterSystem("Input", new Input::InputSystem());
		systemManager.RegisterSystem("Script", new SDE::ScriptSystem());
		systemManager.RegisterSystem("Config", new SDE::ConfigSystem());
		systemManager.RegisterSystem("DebugGui", new DebugGui::DebugGuiSystem());
		systemManager.RegisterSystem("Playground", new Playground());
		systemManager.RegisterSystem("Render", new SDE::RenderSystem());
	}
};

int main()
{
	SystemRegistration sysRegistration;
	return Engine::Run(sysRegistration, 0, nullptr);
}
