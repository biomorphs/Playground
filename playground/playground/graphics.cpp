#include "graphics.h"
#include "kernel/log.h"
#include "core/system_enumerator.h"
#include "debug_gui/debug_gui_system.h"
#include "render/render_pass.h"
#include "sde/script_system.h"
#include "sde/render_system.h"
#include "sde/debug_camera_controller.h"
#include "sde/job_system.h"
#include "input/input_system.h"
#include "render/window.h"
#include "smol/texture_manager.h"
#include "smol/model_manager.h"
#include "smol/shader_manager.h"
#include "smol/renderer.h"
#include "smol/renderer_2d.h"
#include "core/profiler.h"
#include <sol.hpp>

Graphics::Graphics()
{
}

Graphics::~Graphics()
{
}

bool Graphics::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	SDE_PROF_EVENT();

	m_scriptSystem = (SDE::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_renderSystem = (SDE::RenderSystem*)systemEnumerator.GetSystem("Render");
	m_inputSystem = (Input::InputSystem*)systemEnumerator.GetSystem("Input");
	m_jobSystem = (SDE::JobSystem*)systemEnumerator.GetSystem("Jobs");
	m_debugGui = (DebugGui::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");

	return true;
}

bool Graphics::PostInit()
{
	SDE_PROF_EVENT();

	// Create managers
	m_shaders = std::make_unique<smol::ShaderManager>();
	m_textures = std::make_unique<smol::TextureManager>(m_jobSystem);
	m_models = std::make_unique<smol::ModelManager>(m_textures.get(), m_jobSystem);

	const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
	auto windowSize = glm::vec2(windowProps.m_sizeX, windowProps.m_sizeY);

	// Init render passes
	m_render2d = std::make_unique<smol::Renderer2D>(m_textures.get(), windowSize);
	m_renderSystem->AddPass(*m_render2d);
	
	m_render3d = std::make_unique<smol::Renderer>(m_textures.get(), m_models.get(), m_shaders.get(), windowSize);
	m_renderSystem->AddPass(*m_render3d);

	// expose TextureHandle to lua
	m_scriptSystem->Globals().new_usertype<smol::TextureHandle>("TextureHandle",sol::constructors<smol::TextureHandle()>());

	// expose ModelHandle to lua
	m_scriptSystem->Globals().new_usertype<smol::ModelHandle>("ModelHandle",sol::constructors<smol::ModelHandle()>());	

	// expose ShaderHandle to lua
	m_scriptSystem->Globals().new_usertype<smol::ModelHandle>("ShaderHandle", sol::constructors<smol::ShaderHandle()>());

	// expose Graphics namespace functions
	auto graphics = m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
	graphics["SetClearColour"] = [this](float r, float g, float b) {
		m_renderSystem->SetClearColour(glm::vec4(r, g, b, 1.0f));
	};
	graphics["DrawQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a) {
		m_render2d->SubmitQuad({ px,py }, { sx,sy }, { r,g,b,a }, { 0 });
	};
	graphics["DrawTexturedQuad"] = [this](float px, float py, float sx, float sy, float r, float g, float b, float a, smol::TextureHandle h) {
		m_render2d->SubmitQuad({ px,py }, { sx,sy }, { r,g,b,a }, h);
	};
	graphics["LoadTexture"] = [this](const char* path) -> smol::TextureHandle {
		return m_textures->LoadTexture(path);
	};
	graphics["LoadModel"] = [this](const char* path) -> smol::ModelHandle {
		return m_models->LoadModel(path);
	};
	graphics["LoadShader"] = [this](const char* name, const char* vsPath, const char* fsPath) -> smol::ShaderHandle {
		return m_shaders->LoadShader(name, vsPath, fsPath);
	};
	graphics["DrawModel"] = [this](float px, float py, float pz, float r, float g, float b, float a, float scale, smol::ModelHandle h, smol::ShaderHandle sh) {
		auto transform = glm::scale(glm::translate(glm::identity<glm::mat4>(), glm::vec3(px, py, pz)), glm::vec3(scale));
		m_render3d->SubmitInstance(transform, glm::vec4(r,g,b,a), h, sh);
	};
	graphics["SetLight"] = [this](float px, float py, float pz, float r, float g, float b, float ambient) {
		m_render3d->SetLight(glm::vec3(px,py,pz),glm::vec3(r, g, b), ambient);
	};

	m_debugCameraController = std::make_unique<SDE::DebugCameraController>();
	m_debugCameraController->SetPosition({0.0f,0.0f,0.0f});

	return true;
}

bool Graphics::Tick()
{
	SDE_PROF_EVENT();

	// Process loaded data on main thread
	m_textures->ProcessLoadedTextures();
	m_models->ProcessLoadedModels();

	// Use debug camera
	Render::Camera c;
	m_debugCameraController->Update(m_inputSystem->ControllerState(0), 0.033f);
	m_debugCameraController->ApplyToCamera(c);
	m_render3d->SetCamera(c);

	// Debug gui
	bool forceOpen = true;
	m_debugGui->BeginWindow(forceOpen, "Shaders");
	if (m_debugGui->Button("Reload"))
	{
		m_shaders->ReloadAll();
	}
	m_debugGui->EndWindow();

	return true;
}

void Graphics::Shutdown()
{
	SDE_PROF_EVENT();

	m_scriptSystem->Globals()["Graphics"] = nullptr;
	m_render2d = nullptr;
	m_render3d = nullptr;
	m_models = nullptr;
	m_shaders = nullptr;
	m_textures = nullptr;
}