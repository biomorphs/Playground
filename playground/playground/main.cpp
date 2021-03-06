#include "sde/render_system.h"
#include "sde/job_system.h"
#include "sde/script_system.h"
#include "sde/config_system.h"
#include "debug_gui/debug_gui_system.h"
#include "engine/engine_startup.h"
#include "input/input_system.h"
#include "core/system_registrar.h"
#include "playground.h"
#include "graphics.h"

class AppSystems : public Engine::SystemCreator
{
public:
	void Create(Core::ISystemRegistrar& systemManager)
	{
		systemManager.RegisterSystem("Jobs", new SDE::JobSystem());
		systemManager.RegisterSystem("Input", new Input::InputSystem());
		systemManager.RegisterSystem("Script", new SDE::ScriptSystem());
		systemManager.RegisterSystem("Config", new SDE::ConfigSystem());
		systemManager.RegisterSystem("DebugGui", new DebugGui::DebugGuiSystem());
		systemManager.RegisterSystem("Playground", new Playground());
		systemManager.RegisterSystem("Graphics", new Graphics());
		systemManager.RegisterSystem("Render", new SDE::RenderSystem());
	}
};


int main()
{
	AppSystems s;
	return Engine::Run(s, 0, nullptr);
}
