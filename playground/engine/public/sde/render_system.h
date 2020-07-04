/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "core/system.h"
#include "math/glm_headers.h"
#include "render/render_pass.h"
#include <vector>
#include <memory>
#include <string>

namespace Render
{
	class Device;
	class Window;
	class Camera;
	class RenderPass;
}

namespace SDE
{
	class ConfigSystem;

	// Main renderer. Runs on the main thread and handles
	// collection of render primitives, and submission to GPU
	class RenderSystem : public Core::ISystem
	{
	public:
		RenderSystem();
		virtual ~RenderSystem();

		void AddPass(Render::RenderPass& pass, uint32_t sortKey = -1);

		inline Render::Window* GetWindow() { return m_window.get(); }
		inline Render::Device* GetDevice() { return m_device.get(); }

		bool PreInit(Core::ISystemEnumerator& systemEnumerator);
		bool Initialise();		// Window and device are created here
		bool PostInit();		// Window made visible
		bool Tick();			// All passes are drawn here
		void PostShutdown();	// Device + window shutdown here

		inline void SetClearColour(const glm::vec4& c) { m_clearColour = c; }

	private:
		void LoadConfig(ConfigSystem* cfg);

		struct Config
		{
			uint32_t m_windowWidth = 1280;
			uint32_t m_windowHeight = 720;
			std::string m_windowTitle = "SDE";
			bool m_fullscreen = false;
		};

		Config m_config;
		glm::vec4 m_clearColour;

		struct Pass { Render::RenderPass* m_pass; uint32_t m_key; };
		std::vector<Pass> m_passes;
		uint32_t m_lastSortKey = 0;

		std::unique_ptr<Render::Window> m_window;
		std::unique_ptr<Render::Device> m_device;
	};
}