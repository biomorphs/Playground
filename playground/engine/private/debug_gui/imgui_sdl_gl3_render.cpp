#include "imgui_sdl_gl3_render.h"
#include "render/window.h"
#include "render/device.h"
#include "kernel/assert.h"
#include "core/profiler.h"
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

namespace DebugGui
{
	ImguiSdlGL3RenderPass::ImguiSdlGL3RenderPass(Render::Window* window, Render::Device* device)
		: m_window(window)
	{
		SDE_PROF_EVENT();
		bool sdlInitOk = ImGui_ImplSDL2_InitForOpenGL(window->GetWindowHandle(), device->GetGLContext());
		bool glInitOk = ImGui_ImplOpenGL3_Init("#version 130");
		SDE_ASSERT(sdlInitOk, "Imgui SDL Init failed");
		SDE_ASSERT(glInitOk, "Imgui GL Init failed");
	}
	
	ImguiSdlGL3RenderPass::~ImguiSdlGL3RenderPass()
	{
		SDE_PROF_EVENT();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
	}

	void ImguiSdlGL3RenderPass::HandleEvent(void* e)
	{
		SDE_PROF_EVENT();
		ImGui_ImplSDL2_ProcessEvent((SDL_Event*)e);
	}

	void ImguiSdlGL3RenderPass::NewFrame()
	{
		SDE_PROF_EVENT();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(m_window->GetWindowHandle());
	}
	
	void ImguiSdlGL3RenderPass::Reset() 
	{
	}
	
	void ImguiSdlGL3RenderPass::RenderAll(class Render::Device&) 
	{
		SDE_PROF_EVENT();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}