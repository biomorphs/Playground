/*
SDLEngine
Matt Hoyle
*/
#include "render_system.h"
#include "kernel/assert.h"
#include "core/system_enumerator.h"
#include "render/window.h"
#include "render/device.h"
#include "sde/config_system.h"
#include "core/profiler.h"

namespace SDE
{
	RenderSystem::RenderSystem()
	{
	}

	RenderSystem::~RenderSystem()
	{
	}

	void RenderSystem::LoadConfig(ConfigSystem* cfg)
	{
		SDE_PROF_EVENT();

		auto values = cfg->Values();
		auto render = values["Render"];
		if (render.valid())
		{
			auto resolution = render["Resolution"];
			m_config.m_windowWidth = resolution[1].get_or(1280);
			m_config.m_windowHeight = resolution[2].get_or(720);
			m_config.m_fullscreen = render["Fullscreen"].get_or(false);
			m_config.m_windowTitle = render["WindowTitle"].get_or(std::string("SDE"));
		}
	}

	bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
	{
		SDE_PROF_EVENT();
		auto configSystem = (SDE::ConfigSystem*)systemEnumerator.GetSystem("Config");
		LoadConfig(configSystem);

		return true;
	}

	bool RenderSystem::Initialise()
	{
		SDE_PROF_EVENT();

		Render::Window::Properties winProps(m_config.m_windowTitle, m_config.m_windowWidth, m_config.m_windowHeight);
		winProps.m_flags = m_config.m_fullscreen ? Render::Window::CreateFullscreen : 0;
		m_window = std::make_unique<Render::Window>(winProps);
		if (!m_window)
		{
			SDE_LOGC(SDE, "Failed to create window");
			return false;
		}

		m_device = std::make_unique<Render::Device>(*m_window);
		if (!m_device)
		{
			SDE_LOGC(SDE, "Failed to create render device");
			return false;
		}

		return true;
	}

	bool RenderSystem::PostInit()
	{
		SDE_PROF_EVENT();
		m_window->Show();
		return true;
	}

	bool RenderSystem::Tick()
	{
		SDE_PROF_EVENT();

		// bind backbuffer for drawing
		m_device->DrawToBackbuffer();
		m_device->SetViewport({ 0,0 }, { m_config.m_windowWidth, m_config.m_windowHeight });

		for (auto renderPass : m_passes)
		{
			renderPass.m_pass->RenderAll(*m_device);
			renderPass.m_pass->Reset();
		}

		m_device->Present();

		return true;
	}

	void RenderSystem::PostShutdown()
	{
		SDE_PROF_EVENT();

		m_passes.clear();
		m_device = nullptr;
		m_window = nullptr;
	}

	void RenderSystem::AddPass(Render::RenderPass& pass, uint32_t sortKey)
	{
		if (sortKey == -1)
		{
			sortKey = m_lastSortKey++;
		}
		m_passes.push_back({ &pass, sortKey });
		std::sort(m_passes.begin(), m_passes.end(), [](const Pass& a, Pass& b) {
			return a.m_key < b.m_key;
		});
	}
}