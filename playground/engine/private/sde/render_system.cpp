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

namespace SDE
{
	RenderSystem::RenderSystem()
		: m_clearColour(0.0f, 0.0f, 0.0f, 1.0f)
	{
	}

	RenderSystem::~RenderSystem()
	{
	}

	void RenderSystem::LoadConfig(ConfigSystem* cfg)
	{
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
		auto configSystem = (SDE::ConfigSystem*)systemEnumerator.GetSystem("Config");
		LoadConfig(configSystem);

		return true;
	}

	bool RenderSystem::Initialise()
	{
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
		m_window->Show();
		return true;
	}

	bool RenderSystem::Tick()
	{
		m_device->ClearColourDepthTarget(m_clearColour);

		for (auto renderPass : m_passes)
		{
			renderPass->RenderAll(*m_device);
			renderPass->Reset();
		}

		m_device->Present();

		return true;
	}

	void RenderSystem::PostShutdown()
	{
		m_passes.clear();
		m_device = nullptr;
		m_window = nullptr;
	}

	uint32_t RenderSystem::AddPass(Render::RenderPass& pass)
	{
		m_passes.push_back(&pass);
		return (uint32_t)(m_passes.size() - 1);
	}
}